/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBTEXTCLASSIFIER_UTIL_CALENDAR_TYPES_H_
#define LIBTEXTCLASSIFIER_UTIL_CALENDAR_TYPES_H_

struct DateParseData {
  enum Relation {
    NEXT = 1,
    NEXT_OR_SAME = 2,
    LAST = 3,
    NOW = 4,
    TOMORROW = 5,
    YESTERDAY = 6,
    PAST = 7,
    FUTURE = 8
  };

  enum RelationType {
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 7,
    DAY = 8,
    WEEK = 9,
    MONTH = 10,
    YEAR = 11
  };

  enum Fields {
    YEAR_FIELD = 1 << 0,
    MONTH_FIELD = 1 << 1,
    DAY_FIELD = 1 << 2,
    HOUR_FIELD = 1 << 3,
    MINUTE_FIELD = 1 << 4,
    SECOND_FIELD = 1 << 5,
    AMPM_FIELD = 1 << 6,
    ZONE_OFFSET_FIELD = 1 << 7,
    DST_OFFSET_FIELD = 1 << 8,
    RELATION_FIELD = 1 << 9,
    RELATION_TYPE_FIELD = 1 << 10,
    RELATION_DISTANCE_FIELD = 1 << 11
  };

  enum AMPM { AM = 0, PM = 1 };

  enum TimeUnit {
    DAYS = 1,
    WEEKS = 2,
    MONTHS = 3,
    HOURS = 4,
    MINUTES = 5,
    SECONDS = 6,
    YEARS = 7
  };

  // Bit mask of fields which have been set on the struct
  int field_set_mask;

  // Fields describing absolute date fields.
  // Year of the date seen in the text match.
  int year;
  // Month of the year starting with January = 1.
  int month;
  // Day of the month starting with 1.
  int day_of_month;
  // Hour of the day with a range of 0-23,
  // values less than 12 need the AMPM field below or heuristics
  // to definitively determine the time.
  int hour;
  // Hour of the day with a range of 0-59.
  int minute;
  // Hour of the day with a range of 0-59.
  int second;
  // 0 == AM, 1 == PM
  int ampm;
  // Number of hours offset from UTC this date time is in.
  int zone_offset;
  // Number of hours offest for DST
  int dst_offset;

  // The permutation from now that was made to find the date time.
  Relation relation;
  // The unit of measure of the change to the date time.
  RelationType relation_type;
  // The number of units of change that were made.
  int relation_distance;
};

#endif  // LIBTEXTCLASSIFIER_UTIL_CALENDAR_TYPES_H_
