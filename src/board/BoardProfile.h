#ifndef BOARDPROFILE_H
#define BOARDPROFILE_H

/* Numbers that depend on the board and its link. Every tool sets its own: a
 * 38400 baud serial link cannot take a poll rate that USB can. These values
 * suit the fake board only and have never been checked on hardware. */
struct BoardProfile {
    int pollPeriodMs = 200;
    int reconnectRetryMs = 3000;
    int stopConfirmWindowMs = 2000; /* how long the board may stay busy after STOP ALL */
    int moveStartWindowMs = 1500;   /* must be longer than pollPeriodMs */
    int moveTimeoutMs = 30000;
    int minSteps = 0;
    int maxSteps = 10000;
};

#endif // BOARDPROFILE_H
