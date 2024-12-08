#ifndef MY_NTP_H
#define MY_NTP_H

//this function initializes ntp it'S own ntp structure. Only cyw432 needs to initialized first!
void run_ntp(struct tm *time);

#endif