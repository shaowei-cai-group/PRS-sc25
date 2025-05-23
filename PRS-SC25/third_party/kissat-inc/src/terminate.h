#ifndef _terminate_h_INCLUDED
#define _terminate_h_INCLUDED

#include "internal.h"

#ifndef QUIET
void kissat_report_termination (kissat *, int bit, const char *file,
				long lineno, const char *fun);
#endif

static inline bool
kissat_terminated (kissat * solver, int bit,
		   const char *file, long lineno, const char *fun)
{
  assert (0 <= bit), assert (bit < 32);
#ifdef COVERAGE
  const unsigned mask = 1u << bit;
  if (!(solver->terminate & mask))
    return false;
  solver->terminate = ~(unsigned) 0;
#else
  if (!solver->terminate)
    return false;
#endif
#ifndef QUIET
  kissat_report_termination (solver, bit, file, lineno, fun);
#else
  (void) file;
  (void) lineno;
  (void) fun;
#ifndef COVERAGE
  (void) bit;
#endif
#endif
  return true;
}

#define TERMINATED(BIT) \
  kissat_terminated (solver, BIT, __FILE__, __LINE__, __func__)

#endif


#define backbone_terminated_1 1
#define backbone_terminated_2 2
#define backbone_terminated_3 3
#define congruence_terminated_1 4
#define congruence_terminated_2 5
#define congruence_terminated_3 6
#define congruence_terminated_4 7
#define congruence_terminated_5 8
#define congruence_terminated_6 9
#define congruence_terminated_7 10
#define congruence_terminated_8 11
#define congruence_terminated_9 12
#define congruence_terminated_10 13
#define congruence_terminated_11 14
#define congruence_terminated_12 15
#define eliminate_terminated_1 16
#define eliminate_terminated_2 17
#define factor_terminated_1 18
#define fastel_terminated_1 19
#define forward_terminated_1 20
#define kitten_terminated_1 21
#define kitten_terminated_2 22
#define preprocess_terminated_1 23
#define search_terminated_1 24
#define substitute_terminated_1 25
#define sweep_terminated_1 26
#define sweep_terminated_2 27
#define sweep_terminated_3 28
#define sweep_terminated_4 29
#define sweep_terminated_5 30
#define sweep_terminated_6 31
#define sweep_terminated_7 32
#define sweep_terminated_8 33
#define transitive_terminated_1 34
#define transitive_terminated_2 35
#define transitive_terminated_3 36
#define vivify_terminated_1 37
#define vivify_terminated_2 38
#define vivify_terminated_3 39
#define vivify_terminated_4 40
#define vivify_terminated_5 41
#define walk_terminated_1 42
#define warmup_terminated_1 43
