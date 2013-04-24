/*Copyright (c) 2013 Bit Hangar
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "timer.h"

static AppContextRef app_context;
static AppTimerHandle timerHandle;

static long totalSeconds = 0;
static long diffSeconds = 0;
static long displaySeconds = 0;
static int dSeconds = 0;
static int dSecondsOffset = 0;

static bool isRunning = false;
static bool maxTimeReached = false;

static PblTm startTime;

static long MAX_SECONDS = 359999;



void init_timer(AppContextRef ctx, AppTimerHandle timer_handle, long maxSeconds) {
	MAX_SECONDS = maxSeconds;
	app_context = ctx;
	timerHandle = timer_handle;
}

bool is_timer_running() {
	return isRunning;
}

void set_timer_running(bool running) {
	if (running)
		isRunning = true;
	else
		isRunning = false;

}

bool is_max_time_reached() {
	return maxTimeReached;
}

void timer_start() {
	isRunning = true;
	
	//Used to compensate for counter drift in get_display_time
    get_time(&startTime);
    diffSeconds = 0;

    //Account for decisecond start/stop in the middle of a count
    dSecondsOffset = dSeconds;
}

void timer_stop() {
	isRunning = false;
	totalSeconds += diffSeconds;
}

void reset() {
	//clear out static variables
    totalSeconds = 0;
    diffSeconds = 0;
    displaySeconds = 0;
    dSeconds = 0;
    dSecondsOffset = 0;
    maxTimeReached = false;

    get_time(&startTime);
}

int update_timer() {
	 //Increment decisecond, 0-9
    dSeconds++;
    dSeconds = dSeconds % 10;
    return dSeconds;
}

long PblTm_to_seconds(PblTm myTimer) {
    return myTimer.tm_sec + 
        (myTimer.tm_min * 60) + 
        (myTimer.tm_hour * 3600) + 
        (myTimer.tm_yday * 86400) + 
        (myTimer.tm_year * 31536000);    
}

PblTm seconds_to_PblTm(long seconds) {
    //create a PblTm object to store hours minutes seconds,
    //so that we can use Pebble string_format_time function
    PblTm outTime;
    outTime.tm_hour = seconds / 3600;
    seconds %= 3600;

    outTime.tm_min = seconds / 60;
    seconds %= 60;

    outTime.tm_sec = seconds;

    return outTime;
}

long get__current_seconds() {
	return displaySeconds;
}

PblTm get_pebble_time() {
	return seconds_to_PblTm(displaySeconds);
}

void stop_timer() {
	app_timer_cancel_event(app_context, timerHandle);
    isRunning = false;
    maxTimeReached = true;
}

//Display time and recalculate based on Pebble Time
//This is based on the difference of start and current time from the Pebble
PblTm get_display_time() {

    //get the current time
    PblTm currentTime;
    get_time(&currentTime);

    //convert start time and current time to seconds for easier difference calculation
    long startTimeSeconds = PblTm_to_seconds(startTime);
    long currentTimeSeconds = PblTm_to_seconds(currentTime);

    //find difference in seconds
    diffSeconds = currentTimeSeconds - startTimeSeconds;

    //Remember to use decisecond offsest
    if (diffSeconds == 0 && dSecondsOffset > 0)
        totalSeconds++;

    //Add total running time (totalSeconds) to display time
    displaySeconds = totalSeconds + diffSeconds;

    // Stop the count if max reached
    if (displaySeconds >= MAX_SECONDS)
    {
       	stop_timer();
    	PblTm zeroTime = { .tm_sec = 0, .tm_min = 0, .tm_hour = 0};
    	return zeroTime;
    }

    return seconds_to_PblTm(displaySeconds);
}

