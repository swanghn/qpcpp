//$file${.::mine2.cpp} #######################################################
//
// Model: game.qm
// File:  ${.::mine2.cpp}
//
// This code has been generated by QM 4.3.1 (https://www.state-machine.com/qm).
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
//$endhead${.::mine2.cpp} ####################################################
#include "qpcpp.h"
#include "bsp.h"
#include "game.h"

Q_DEFINE_THIS_FILE

// encapsulated delcaration of the Mine1 HSM ---------------------------------
//$declare${AOs::Mine2} ######################################################
namespace GAME {

//${AOs::Mine2} ..............................................................
class Mine2 : public QP::QHsm {
private:
    uint8_t m_x;
    uint8_t m_y;
    uint8_t m_exp_ctr;

public:
    Mine2()
      : QHsm(Q_STATE_CAST(&Mine2::initial))
    {}

protected:
    static QP::QState initial(Mine2 * const me, QP::QEvt const * const e);
    static QP::QState unused(Mine2 * const me, QP::QEvt const * const e);
    static QP::QState used(Mine2 * const me, QP::QEvt const * const e);
    static QP::QState exploding(Mine2 * const me, QP::QEvt const * const e);
    static QP::QState planted(Mine2 * const me, QP::QEvt const * const e);
};

} // namespace GAME
//$enddecl${AOs::Mine2} ######################################################

namespace GAME {

// local objects -------------------------------------------------------------
static Mine2 l_mine2[GAME_MINES_MAX]; // a pool of type-2 mines

//............................................................................
QP::QHsm *Mine2_getInst(uint8_t id) {
    Q_REQUIRE(id < GAME_MINES_MAX);
    return &l_mine2[id];
}

// helper function to provide the ID of this mine ............................
static inline uint8_t MINE_ID(Mine2 const * const me) {
    return static_cast<uint8_t>(me - l_mine2);
}

} // namespace GAME

// Mine1 class definition ----------------------------------------------------
// Check for the minimum required QP version
#if (QP_VERSION < 630U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 6.3.0 or higher required
#endif

//$define${AOs::Mine2} #######################################################
namespace GAME {

//${AOs::Mine2} ..............................................................
//${AOs::Mine2::SM} ..........................................................
QP::QState Mine2::initial(Mine2 * const me, QP::QEvt const * const e) {
    //${AOs::Mine2::SM::initial}
    static bool dict_sent = false;
    if (!dict_sent) {
        dict_sent = true;
        // object dictionaries for Mine2 pool...
        QS_OBJ_DICTIONARY(&l_mine2[0]);
        QS_OBJ_DICTIONARY(&l_mine2[1]);
        QS_OBJ_DICTIONARY(&l_mine2[2]);
        QS_OBJ_DICTIONARY(&l_mine2[3]);
        QS_OBJ_DICTIONARY(&l_mine2[4]);

        // function dictionaries for Mine2 SM...
        QS_FUN_DICTIONARY(&Mine2::initial);
        QS_FUN_DICTIONARY(&Mine2::unused);
        QS_FUN_DICTIONARY(&Mine2::used);
        QS_FUN_DICTIONARY(&Mine2::planted);
        QS_FUN_DICTIONARY(&Mine2::exploding);
    }
    // local signals...
    QS_SIG_DICTIONARY(MINE_PLANT_SIG,    me);
    QS_SIG_DICTIONARY(MINE_DISABLED_SIG, me);
    QS_SIG_DICTIONARY(MINE_RECYCLE_SIG,  me);
    QS_SIG_DICTIONARY(SHIP_IMG_SIG,      me);
    QS_SIG_DICTIONARY(MISSILE_IMG_SIG,   me);

    (void)e; // unused parameter
    return Q_TRAN(&unused);
}
//${AOs::Mine2::SM::unused} ..................................................
QP::QState Mine2::unused(Mine2 * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Mine2::SM::unused::MINE_PLANT}
        case MINE_PLANT_SIG: {
            me->m_x = Q_EVT_CAST(ObjectPosEvt)->x;
            me->m_y = Q_EVT_CAST(ObjectPosEvt)->y;
            status_ = Q_TRAN(&planted);
            break;
        }
        default: {
            status_ = Q_SUPER(&top);
            break;
        }
    }
    return status_;
}
//${AOs::Mine2::SM::used} ....................................................
QP::QState Mine2::used(Mine2 * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Mine2::SM::used}
        case Q_EXIT_SIG: {
            // tell the Tunnel that this mine is becoming disabled
            MineEvt *mev = Q_NEW(MineEvt, MINE_DISABLED_SIG);
            mev->id = MINE_ID(me);
            AO_Tunnel->POST(mev, me);
            status_ = Q_HANDLED();
            break;
        }
        //${AOs::Mine2::SM::used::MINE_RECYCLE}
        case MINE_RECYCLE_SIG: {
            status_ = Q_TRAN(&unused);
            break;
        }
        default: {
            status_ = Q_SUPER(&top);
            break;
        }
    }
    return status_;
}
//${AOs::Mine2::SM::used::exploding} .........................................
QP::QState Mine2::exploding(Mine2 * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Mine2::SM::used::exploding}
        case Q_ENTRY_SIG: {
            me->m_exp_ctr = 0U;
            status_ = Q_HANDLED();
            break;
        }
        //${AOs::Mine2::SM::used::exploding::TIME_TICK}
        case TIME_TICK_SIG: {
            //${AOs::Mine2::SM::used::exploding::TIME_TICK::[stillonscreen?]}
            if ((me->m_x >= GAME_SPEED_X) && (me->m_exp_ctr < 15)) {
                ++me->m_exp_ctr;  // advance the explosion counter
                me->m_x -= GAME_SPEED_X; // move explosion by 1 step

                // tell the Game to render the current stage of Explosion
                ObjectImageEvt *oie = Q_NEW(ObjectImageEvt, EXPLOSION_SIG);
                oie->x   = me->m_x + 1U;  // x of explosion
                oie->y   = (int8_t)((int)me->m_y - 4 + 2); // y of explosion
                oie->bmp = EXPLOSION0_BMP + (me->m_exp_ctr >> 2);
                AO_Tunnel->POST(oie, me);
                status_ = Q_HANDLED();
            }
            //${AOs::Mine2::SM::used::exploding::TIME_TICK::[else]}
            else {
                status_ = Q_TRAN(&unused);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&used);
            break;
        }
    }
    return status_;
}
//${AOs::Mine2::SM::used::planted} ...........................................
QP::QState Mine2::planted(Mine2 * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Mine2::SM::used::planted::TIME_TICK}
        case TIME_TICK_SIG: {
            //${AOs::Mine2::SM::used::planted::TIME_TICK::[me->m_x>=GAME_SPEED_X]}
            if (me->m_x >= GAME_SPEED_X) {
                me->m_x -= GAME_SPEED_X; // move the mine 1 step
                // tell the Tunnel to draw the Mine
                ObjectImageEvt *oie = Q_NEW(ObjectImageEvt, MINE_IMG_SIG);
                oie->x   = me->m_x;
                oie->y   = me->m_y;
                oie->bmp = MINE2_BMP;
                AO_Tunnel->POST(oie, me);
                status_ = Q_HANDLED();
            }
            //${AOs::Mine2::SM::used::planted::TIME_TICK::[else]}
            else {
                status_ = Q_TRAN(&unused);
            }
            break;
        }
        //${AOs::Mine2::SM::used::planted::SHIP_IMG}
        case SHIP_IMG_SIG: {
            uint8_t x   = Q_EVT_CAST(ObjectImageEvt)->x;
            uint8_t y   = Q_EVT_CAST(ObjectImageEvt)->y;
            uint8_t bmp = Q_EVT_CAST(ObjectImageEvt)->bmp;
            //${AOs::Mine2::SM::used::planted::SHIP_IMG::[collisionwithMINE2_BMP?]}
            if (BSP_doBitmapsOverlap(MINE2_BMP, me->m_x, me->m_y, bmp, x, y)) {
                static MineEvt const mine2_hit(HIT_MINE_SIG, 2U);
                AO_Ship->POST(&mine2_hit, me);
                // go straight to 'disabled' and let the Ship do
                // the exploding
                status_ = Q_TRAN(&unused);
            }
            else {
                status_ = Q_UNHANDLED();
            }
            break;
        }
        //${AOs::Mine2::SM::used::planted::MISSILE_IMG}
        case MISSILE_IMG_SIG: {
            uint8_t x   = Q_EVT_CAST(ObjectImageEvt)->x;
            uint8_t y   = Q_EVT_CAST(ObjectImageEvt)->y;
            uint8_t bmp = Q_EVT_CAST(ObjectImageEvt)->bmp;
            //${AOs::Mine2::SM::used::planted::MISSILE_IMG::[collisionwithMINE2_MISSILE_BMP?~}
            if (BSP_doBitmapsOverlap(MINE2_MISSILE_BMP, me->m_x, me->m_y, bmp, x, y)) {
                static ScoreEvt const mine2_destroyed(DESTROYED_MINE_SIG, 45U);
                AO_Missile->POST(&mine2_destroyed, me);
                status_ = Q_TRAN(&exploding);
            }
            else {
                status_ = Q_UNHANDLED();
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&used);
            break;
        }
    }
    return status_;
}

} // namespace GAME
//$enddef${AOs::Mine2} #######################################################
