#include "board/BoardWorker.h"

BoardWorker::BoardWorker(MakeLink makeLink, QObject *parent)
    : QObject(parent), m_makeLink(std::move(makeLink))
{
    m_thread = QThread::create([this]() { runLoop(); });
    m_thread->setObjectName("BoardWorker");
    m_thread->start();
}

BoardWorker::~BoardWorker()
{
    {
        QMutexLocker lock(&m_mutex);
        m_jobs.clear();
        m_isEnding = true;
        m_hasWork.wakeAll();
    }
    m_thread->wait();
    delete m_thread;
}

void BoardWorker::open(QObject *context, std::function<void(const OpenResult &)> done)
{
    call<OpenResult>(
        context,
        [](BoardLink &link) {
            OpenResult result;
            if (!link.isOpen() && !link.open(result.whyNot)) {
                return result;
            }
            const BoardReply reply = link.readVersion(result.version);
            if (!reply.isOk()) {
                link.close();
                result.whyNot = reply.text();
                return result;
            }
            result.isOpen = true;
            return result;
        },
        std::move(done));
}

void BoardWorker::close()
{
    const int dropped = dropWaitingJobs();
    queue(
        [this](BoardLink &link) {
            m_isCloseAsked = true;
            link.close();
        },
        false);
    if (dropped > 0) {
        emit jobsDropped(dropped);
    }
}

int BoardWorker::dropWaitingJobs()
{
    QMutexLocker lock(&m_mutex);
    const int    dropped = static_cast<int>(m_jobs.size());
    m_jobs.clear();
    return dropped;
}

void BoardWorker::queue(Job job, bool isFirst)
{
    int dropped = 0;
    if (isFirst) {
        dropped = dropWaitingJobs();
    }
    {
        QMutexLocker lock(&m_mutex);
        m_jobs.push_back(std::move(job));
        m_hasWork.wakeOne();
    }
    if (dropped > 0) {
        emit jobsDropped(dropped);
    }
}

void BoardWorker::runLoop()
{
    /* Made here so a link built on QSerialPort lives on this thread. */
    std::unique_ptr<BoardLink> link = m_makeLink();

    for (;;) {
        Job job;
        {
            QMutexLocker lock(&m_mutex);
            while (m_jobs.empty() && !m_isEnding) {
                m_hasWork.wait(&m_mutex);
            }
            if (m_isEnding) {
                break;
            }
            job = std::move(m_jobs.front());
            m_jobs.pop_front();
        }
        const bool wasOpen = link->isOpen();
        job(*link);
        const bool isLost = wasOpen && !link->isOpen() && !m_isCloseAsked;
        m_isCloseAsked = false;
        if (isLost) {
            /* A move still waiting must not run once the link is back. */
            const int dropped = dropWaitingJobs();
            if (dropped > 0) {
                emit jobsDropped(dropped);
            }
            emit connectionLost();
        }
    }
    link->close();
}
