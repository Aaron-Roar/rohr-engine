/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "elderfly_descriptors.h"
AnimationDescriptor elderfly_fly = (AnimationDescriptor) {
  .frame_files = {
    "assets/view-port/elder-fly/flying/f1.png",
    "assets/view-port/elder-fly/flying/f2.png",
    "assets/view-port/elder-fly/flying/f3.png",
    "assets/view-port/elder-fly/flying/f4.png",
    "assets/view-port/elder-fly/flying/f5.png",
    "assets/view-port/elder-fly/flying/f6.png",
    "assets/view-port/elder-fly/flying/f7.png",
    "assets/view-port/elder-fly/flying/f8.png",
    "assets/view-port/elder-fly/flying/f9.png",
    "assets/view-port/elder-fly/flying/f10.png",
    "assets/view-port/elder-fly/flying/f11.png",
    "assets/view-port/elder-fly/flying/f12.png",
  },
  .amount_of_descriptors = 12,
  .ticks_per_frame = 0,
  .time_per_frame = 0.05,
};

AnimationDescriptor elderfly_death = (AnimationDescriptor) {
  .frame_files = {
    "assets/view-port/elder-fly/death/d1.png",
    "assets/view-port/elder-fly/death/d2.png",
    "assets/view-port/elder-fly/death/d3.png",
    "assets/view-port/elder-fly/death/d4.png",
    "assets/view-port/elder-fly/death/d5.png",
    "assets/view-port/elder-fly/death/d6.png",
    "assets/view-port/elder-fly/death/d7.png",
    "assets/view-port/elder-fly/death/d8.png",
    "assets/view-port/elder-fly/death/d9.png",
    "assets/view-port/elder-fly/death/d10.png",
  },
  .amount_of_descriptors = 10,
  .ticks_per_frame = 0,
  .time_per_frame = 0.08,
};

AnimationDescriptor elderfly_attack = (AnimationDescriptor) {
  .frame_files = {
    "assets/view-port/elder-fly/attack/a1.png",
    "assets/view-port/elder-fly/attack/a2.png",
    "assets/view-port/elder-fly/attack/a3.png",
    "assets/view-port/elder-fly/attack/a4.png",
    "assets/view-port/elder-fly/attack/a5.png",
    "assets/view-port/elder-fly/attack/a6.png",
    "assets/view-port/elder-fly/attack/a7.png",
    "assets/view-port/elder-fly/attack/a8.png",
    "assets/view-port/elder-fly/attack/a9.png",
  },
  .amount_of_descriptors = 9,
  .ticks_per_frame = 0,
  .time_per_frame = 0.05,
};

AnimationDescriptor elderfly_jump = (AnimationDescriptor) {
  .frame_files = {
    "assets/view-port/elder-fly/jump/j1.png",
    "assets/view-port/elder-fly/jump/j2.png",
    "assets/view-port/elder-fly/jump/j3.png",
    "assets/view-port/elder-fly/jump/j4.png",
    "assets/view-port/elder-fly/jump/j5.png",
    "assets/view-port/elder-fly/jump/j6.png",
    "assets/view-port/elder-fly/jump/j7.png",
    "assets/view-port/elder-fly/jump/j8.png",
    "assets/view-port/elder-fly/jump/j9.png",
  },
  .amount_of_descriptors = 9,
  .ticks_per_frame = 0,
  .time_per_frame = 0.05,
};

AnimationDescriptor elderfly_land = (AnimationDescriptor) {
  .frame_files = {
    "assets/view-port/elder-fly/land/l1.png",
    "assets/view-port/elder-fly/land/l2.png",
    "assets/view-port/elder-fly/land/l3.png",
    "assets/view-port/elder-fly/land/l4.png",
    "assets/view-port/elder-fly/land/l5.png",
  },
  .amount_of_descriptors = 5,
  .ticks_per_frame = 0,
  .time_per_frame = 0.05,
};

AnimationDescriptor elderfly_take_damage = (AnimationDescriptor) {
  .frame_files = {
    "assets/view-port/elder-fly/take-damage/td1.png",
    "assets/view-port/elder-fly/take-damage/td2.png",
    "assets/view-port/elder-fly/take-damage/td3.png",
    "assets/view-port/elder-fly/take-damage/td4.png",
    "assets/view-port/elder-fly/take-damage/td5.png",
    "assets/view-port/elder-fly/take-damage/td6.png",
    "assets/view-port/elder-fly/take-damage/td7.png",
    "assets/view-port/elder-fly/take-damage/td8.png",
    "assets/view-port/elder-fly/take-damage/td9.png",
  },
  .amount_of_descriptors = 9,
  .ticks_per_frame = 0,
  .time_per_frame = 0.05,
};
