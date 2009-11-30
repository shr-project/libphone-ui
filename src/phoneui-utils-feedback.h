#ifndef _PHONEUI_UTILS_FEEDBACK_H
#define _PHONEUI_UTILS_FEEDBACK_H

#define FEEDBACK_VIBRATE_MIN_DURATION 10
#define FEEDBACK_VIBRATE_DEFAULT_INTENSITY 500
#define FEEDBACK_VIBRATE_DEFAULT_PAUSE 10

#define FEEDBACK_FLASH_MIN_DURATION 10
#define FEEDBACK_FLASH_DEFAULT_INTENSITY 500

enum FeedbackAction {
	FEEDBACK_ACTION_CLICK = 0,
	FEEDBACK_ACTION_NOTICE,
	FEEDBACK_ACTION_WARNING,
	FEEDBACK_ACTION_ERROR,
	FEEDBACK_ACTION_END /* must be last */
};

enum FeedbackLevel {
	FEEDBACK_LEVEL_OFF = 0,
	FEEDBACK_LEVEL_LOW,
	FEEDBACK_LEVEL_MEDIUM,
	FEEDBACK_LEVEL_HIGH
};

enum FeedbackLevel feedback_level;

int
phoneui_utils_feedback_init(GKeyFile *keyfile);

void
phoneui_utils_feedback_action(enum FeedbackAction action,
		enum FeedbackLevel level);


#endif

