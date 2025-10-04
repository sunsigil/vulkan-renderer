#pragma once

#define TOS_min(a, b) ((b) < (a) ? (b) : (a))
#define TOS_max(a, b) ((b) > (a) ? (b) : (a))
#define TOS_clamp(x, a, b) TOS_min(TOS_max(x, a), b)
