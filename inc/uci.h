#ifndef UCI_H
#define UCI_H

// Defining time controls
#define RAPID 300000 // 5 min+
#define BLITZ 60000 // 1 min+

// How much of a time lead we have
#define LARGE_LEAD  5000
#define LEAD        0

// When to change strategy
#define DANGER_TIME   12000
#define INSTANT_TIME  5000

// Maximum move times
#define RAPID_MAX  90000
#define BLITZ_MAX  40000
#define BULLET_MAX 8000
#define RAPID_STANDARD  60000
#define BLITZ_STANDARD  15000
#define BULLET_STANDARD 4000
#define RAPID_RUSH  20000
#define BLITZ_RUSH  8000
#define BULLET_RUSH 3000
#define DANGER 200
// #define INSTANT 60

void uci(void);

#endif