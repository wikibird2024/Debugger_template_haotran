#ifndef BOARDWORKER_H
#define BOARDWORKER_H

#include <QMutex>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QWaitCondition>
#include <deque>
#include <functional>
#include <memory>

#include "board/BoardLink.h"

/* The one thread that talks to the board, one job at a time, so the window
 * never freezes and STOP ALL always works.
 *
 *   worker.call<BoardReply>(this,
 *       [](BoardLink &link) { return link.moveTo(100); },   // worker thread
 *       [this](const BoardReply &reply) { show(reply); });  // GUI thread, later
 *
 * - Jobs run in the order they were queued.
 * - callFirst() throws away every waiting job and goes next. It cannot cut
 *   short the exchange in progress, so a stop waits for at most one timeout.
 * - A thrown away job never runs `done`, so every waiting flag must also
 *   reset on jobsDropped() (Session::workEnded does this for pages).
 * - `done` runs on the GUI thread, and only while `context` is alive. */
class BoardWorker : public QObject
{
    Q_OBJECT
  public:
    using MakeLink = std::function<std::unique_ptr<BoardLink>()>;

    struct OpenResult {
        bool    isOpen = false;
        QString whyNot;
        QString version;
    };

    explicit BoardWorker(MakeLink makeLink, QObject *parent = nullptr);
    ~BoardWorker() override;

    /* Opens the link and reads the version. Open means the board answered. */
    void open(QObject *context, std::function<void(const OpenResult &)> done);
    /* Throws away every waiting job, then closes the link. */
    void close();

    template <typename Result>
    void call(QObject *context, std::function<Result(BoardLink &)> job, std::function<void(const Result &)> done = nullptr)
    {
        queue(wrap<Result>(context, std::move(job), std::move(done)), false);
    }

    template <typename Result>
    void callFirst(QObject *context, std::function<Result(BoardLink &)> job, std::function<void(const Result &)> done = nullptr)
    {
        queue(wrap<Result>(context, std::move(job), std::move(done)), true);
    }

  signals:
    void jobsDropped(int count);
    void connectionLost(); /* the link closed by itself; waiting jobs are already thrown away */

  private:
    using Job = std::function<void(BoardLink &)>;

    template <typename Result>
    Job wrap(QObject *context, std::function<Result(BoardLink &)> job, std::function<void(const Result &)> done)
    {
        /* A QPointer may only be read on the thread that deletes its object,
           so `context` is checked on the GUI thread, not here. */
        QPointer<QObject> guard(context);
        return [this, guard, job = std::move(job), done = std::move(done)](BoardLink &link) {
            Result result = job(link);
            if (done) {
                QMetaObject::invokeMethod(
                    this,
                    [guard, done, result]() {
                        if (guard) {
                            done(result);
                        }
                    },
                    Qt::QueuedConnection);
            }
        };
    }

    void queue(Job job, bool isFirst);
    int  dropWaitingJobs();
    void runLoop();

    MakeLink        m_makeLink;
    QThread        *m_thread = nullptr;
    QMutex          m_mutex;
    QWaitCondition  m_hasWork;
    std::deque<Job> m_jobs;                 /* guarded by m_mutex */
    bool            m_isEnding = false;     /* guarded by m_mutex */
    bool            m_isCloseAsked = false; /* worker thread only */
};

#endif // BOARDWORKER_H
