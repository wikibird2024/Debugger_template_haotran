#ifndef MOVELIMITS_H
#define MOVELIMITS_H

#include <QString>

/* PC-side travel limits. Firmware may have none: a raw move goes wherever it
 * is told, so no move leaves the tool without this check. */
class MoveLimits
{
  public:
    MoveLimits(int minSteps, int maxSteps)
        : m_minSteps(minSteps), m_maxSteps(maxSteps)
    {
    }

    bool check(int positionSteps, QString &whyNot) const
    {
        if (positionSteps >= m_minSteps && positionSteps <= m_maxSteps) {
            return true;
        }
        whyNot = QString("%1 steps is outside the limits (%2 to %3)").arg(positionSteps).arg(m_minSteps).arg(m_maxSteps);
        return false;
    }

  private:
    int m_minSteps;
    int m_maxSteps;
};

#endif // MOVELIMITS_H
