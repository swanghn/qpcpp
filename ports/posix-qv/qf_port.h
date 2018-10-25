/// @file
/// @brief QF/C++ port to POSIX API with cooperative QV scheduler (posix-qv)
/// @cond
///***************************************************************************
/// Last updated for version 6.3.6
/// Last updated on  2018-10-20
///
///                    Q u a n t u m  L e a P s
///                    ------------------------
///                    Modern Embedded Software
///
/// Copyright (C) 2005-2018 Quantum Leaps, LLC. All rights reserved.
///
/// This program is open source software: you can redistribute it and/or
/// modify it under the terms of the GNU General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// Alternatively, this program may be distributed and modified under the
/// terms of Quantum Leaps commercial licenses, which expressly supersede
/// the GNU General Public License and are specifically designed for
/// licensees interested in retaining the proprietary status of their code.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program. If not, see <http://www.gnu.org/licenses/>.
///
/// Contact information:
/// https://www.state-machine.com
/// mailto:info@state-machine.com
///***************************************************************************
/// @endcond

#ifndef qf_port_h
#define qf_port_h

// event queue and thread types
#define QF_EQUEUE_TYPE       QEQueue
//#define QF_OS_OBJECT_TYPE  // not used
//#define QF_THREAD_TYPE     // not used

// The maximum number of active objects in the application
#define QF_MAX_ACTIVE        64

// The number of system clock tick rates
#define QF_MAX_TICK_RATE     2

// various QF object sizes configuration for this port
#define QF_EVENT_SIZ_SIZE    4
#define QF_EQUEUE_CTR_SIZE   4
#define QF_MPOOL_SIZ_SIZE    4
#define QF_MPOOL_CTR_SIZE    4
#define QF_TIMEEVT_CTR_SIZE  4

/* QF interrupt disable/enable, see NOTE1 */
#define QF_INT_DISABLE()     pthread_mutex_lock(&QP::QF_pThreadMutex_)
#define QF_INT_ENABLE()      pthread_mutex_unlock(&QP::QF_pThreadMutex_)

// QF critical section entry/exit for POSIX, see NOTE1
// QF_CRIT_STAT_TYPE not defined
#define QF_CRIT_ENTRY(dummy) QF_INT_DISABLE()
#define QF_CRIT_EXIT(dummy)  QF_INT_ENABLE()

// QF_LOG2 not defined -- use the internal LOG2() implementation

#include <pthread.h>   // POSIX-thread API
#include "qep_port.h"  // QEP port
#include "qequeue.h"   // POSIX-QV needs event-queue
#include "qmpool.h"    // POSIX-QV needs memory-pool
#include "qpset.h"     // POSIX-QV needs priority-set
#include "qf.h"        // QF platform-independent public interface

namespace QP {

// set clock tick rate and p-thread priority
// (NOTE: ticksPerSec==0 disables the "ticker thread"
void QF_setTickRate(uint32_t ticksPerSec, int_t tickPrio);

// clock tick callback (NOTE not called when "ticker thread" is not running)
void QF_onClockTick(void);

// abstractions for console access...
void QF_consoleSetup(void);
void QF_consoleCleanup(void);
int  QF_consoleGetKey(void);
int  QF_consoleWaitForKey(void);

extern pthread_mutex_t QF_pThreadMutex_; // mutex for QF critical section

} // namespace QP

//****************************************************************************
// interface used only inside QF implementation, but not in applications
//
#ifdef QP_IMPL

    // scheduler locking (not needed in single-thread port)
    #define QF_SCHED_STAT_
    #define QF_SCHED_LOCK_(dummy) ((void)0)
    #define QF_SCHED_UNLOCK_()    ((void)0)

    // event queue operations...
    #define QACTIVE_EQUEUE_WAIT_(me_) \
        Q_ASSERT((me_)->m_eQueue.m_frontEvt != static_cast<QEvt const *>(0))

    #define QACTIVE_EQUEUE_SIGNAL_(me_) do { \
        QV_readySet_.insert((me_)->m_prio); \
        pthread_cond_signal(&QV_condVar_); \
    } while (false)

    // event pool operations...
    #define QF_EPOOL_TYPE_  QMPool

    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) \
        (p_).init((poolSto_), (poolSize_), (evtSize_))

    #define QF_EPOOL_EVENT_SIZE_(p_)  ((p_).getBlockSize())
    #define QF_EPOOL_GET_(p_, e_, m_) \
        ((e_) = static_cast<QEvt *>((p_).get((m_))))
    #define QF_EPOOL_PUT_(p_, e_)     ((p_).put(e_))

    namespace QP {
        extern QPSet QV_readySet_; // QV-ready set of active objects
        extern pthread_cond_t QV_condVar_; // Cond.var. to signal events
    } // namespace QP

#endif // QP_IMPL

// NOTES: ====================================================================
//
// NOTE1:
// QF, like all real-time frameworks, needs to execute certain sections of
// code indivisibly to avoid data corruption. The most straightforward way of
// protecting such critical sections of code is disabling and enabling
// interrupts, which the POSIX API does not allow.
//
// This QF port uses therefore a single package-scope p-thread mutex
// QF_pThreadMutex_ to protect all critical sections. The mutex is locked upon
// the entry to each critical sectioni and unlocked upon exit.
//
// Using the single mutex for all crtical section guarantees that only one
// thread at a time can execute inside a critical section. This prevents race
// conditions and data corruption.
//
// Please note, however, that the mutex implementation of a critical section
// behaves differently than the standard interrupt locking. A common mutex
// ensures that only one thread at a time can execute a critical section, but
// it does not guarantee that a context switch cannot occur within the
// critical section. In fact, such context switches probably will happen, but
// they should not cause concurrency hazards because the mutex eliminates
// race conditionis.
//
// Unlinke simply disabling and enabling interrupts, the mutex approach is
// also subject to priority inversions. However, the p-thread mutex
// implementation, such as Linux p-threads, should support the priority-
// inheritance protocol.
//

#endif // qf_port_h

