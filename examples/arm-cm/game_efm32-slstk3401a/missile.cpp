//$file${.::missile.cpp} #####################################################
//
// Model: game.qm
// File:  ${.::missile.cpp}
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
//$endhead${.::missile.cpp} ##################################################
#include "qpcpp.h"
#include "bsp.h"
#include "game.h"

//Q_DEFINE_THIS_FILE

//$declare${AOs::Missile} ####################################################
namespace GAME {

//${AOs::Missile} ............................................................
class Missile : public QP::QActive {
private:
    uint8_t m_x;
    uint8_t m_y;
    uint8_t m_exp_ctr;

public:
    Missile();

protected:
    static QP::QState initial(Missile * const me, QP::QEvt const * const e);
    static QP::QState armed(Missile * const me, QP::QEvt const * const e);
    static QP::QState flying(Missile * const me, QP::QEvt const * const e);
    static QP::QState exploding(Missile * const me, QP::QEvt const * const e);
};

} // namespace GAME
//$enddecl${AOs::Missile} ####################################################

namespace GAME {

// local objects -------------------------------------------------------------
static Missile l_missile; // the sole instance of the Missile active object

} // namespace GAME

// Public-scope objects ------------------------------------------------------
// Check for the minimum required QP version
#if (QP_VERSION < 630U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 6.3.0 or higher required
#endif

//$define${AOs::AO_Missile} ##################################################
namespace GAME {


// opaque pointer
//${AOs::AO_Missile} .........................................................
QP::QActive * const AO_Missile = &l_missile;

} // namespace GAME
//$enddef${AOs::AO_Missile} ##################################################

// Active object definition --------------------------------------------------
//$define${AOs::Missile} #####################################################
namespace GAME {

//${AOs::Missile} ............................................................
//${AOs::Missile::Missile} ...................................................
Missile::Missile()
  : QActive(Q_STATE_CAST(&Missile::initial))
{}

//${AOs::Missile::SM} ........................................................
QP::QState Missile::initial(Missile * const me, QP::QEvt const * const e) {
    //${AOs::Missile::SM::initial}
    me->subscribe( TIME_TICK_SIG);

    // local signals...
    QS_SIG_DICTIONARY(MISSILE_FIRE_SIG,   me);
    QS_SIG_DICTIONARY(HIT_WALL_SIG,       me);
    QS_SIG_DICTIONARY(DESTROYED_MINE_SIG, me);

    (void)e; // unused parameter

    QS_FUN_DICTIONARY(&armed);
    QS_FUN_DICTIONARY(&flying);
    QS_FUN_DICTIONARY(&exploding);

    return Q_TRAN(&armed);
}
//${AOs::Missile::SM::armed} .................................................
QP::QState Missile::armed(Missile * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Missile::SM::armed::MISSILE_FIRE}
        case MISSILE_FIRE_SIG: {
            me->m_x = Q_EVT_CAST(ObjectPosEvt)->x;
            me->m_y = Q_EVT_CAST(ObjectPosEvt)->y;
            status_ = Q_TRAN(&flying);
            break;
        }
        default: {
            status_ = Q_SUPER(&top);
            break;
        }
    }
    return status_;
}
//${AOs::Missile::SM::flying} ................................................
QP::QState Missile::flying(Missile * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Missile::SM::flying::TIME_TICK}
        case TIME_TICK_SIG: {
            //${AOs::Missile::SM::flying::TIME_TICK::[me->m_x+GAME_MISSILE_SPEED_X<GA~}
            if (me->m_x + GAME_MISSILE_SPEED_X < GAME_TUNNEL_WIDTH) {
                me->m_x += GAME_MISSILE_SPEED_X;
                // tell the Tunnel to draw the Missile and test for wall hits
                ObjectImageEvt *oie = Q_NEW(ObjectImageEvt, MISSILE_IMG_SIG);
                oie->x   = me->m_x;
                oie->y   = me->m_y;
                oie->bmp = MISSILE_BMP;
                AO_Tunnel->POST(oie, me);
                status_ = Q_HANDLED();
            }
            //${AOs::Missile::SM::flying::TIME_TICK::[else]}
            else {
                status_ = Q_TRAN(&armed);
            }
            break;
        }
        //${AOs::Missile::SM::flying::HIT_WALL}
        case HIT_WALL_SIG: {
            status_ = Q_TRAN(&exploding);
            break;
        }
        //${AOs::Missile::SM::flying::DESTROYED_MINE}
        case DESTROYED_MINE_SIG: {
            AO_Ship->POST(e, me);
            status_ = Q_TRAN(&armed);
            break;
        }
        default: {
            status_ = Q_SUPER(&top);
            break;
        }
    }
    return status_;
}
//${AOs::Missile::SM::exploding} .............................................
QP::QState Missile::exploding(Missile * const me, QP::QEvt const * const e) {
    QP::QState status_;
    switch (e->sig) {
        //${AOs::Missile::SM::exploding}
        case Q_ENTRY_SIG: {
            me->m_exp_ctr = 0U;
            status_ = Q_HANDLED();
            break;
        }
        //${AOs::Missile::SM::exploding::TIME_TICK}
        case TIME_TICK_SIG: {
            //${AOs::Missile::SM::exploding::TIME_TICK::[(me->m_x>=GAME_SPEED_X)&&(me->m~}
            if ((me->m_x >= GAME_SPEED_X) && (me->m_exp_ctr < 15U)) {
                ++me->m_exp_ctr;           // advance the explosion counter
                me->m_x -= GAME_SPEED_X;   // move the explosion by one step

                // tell the Tunnel to render the current stage of Explosion
                ObjectImageEvt *oie = Q_NEW(ObjectImageEvt, EXPLOSION_SIG);
                oie->x   = me->m_x + 3U;   // x-pos of explosion
                oie->y   = (int8_t)((int)me->m_y - 4U); // y-pos
                oie->bmp = EXPLOSION0_BMP + (me->m_exp_ctr >> 2);
                AO_Tunnel->POST(oie, me);
                status_ = Q_HANDLED();
            }
            //${AOs::Missile::SM::exploding::TIME_TICK::[else]}
            else {
                status_ = Q_TRAN(&armed);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&top);
            break;
        }
    }
    return status_;
}

} // namespace GAME
//$enddef${AOs::Missile} #####################################################
