/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 */

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

