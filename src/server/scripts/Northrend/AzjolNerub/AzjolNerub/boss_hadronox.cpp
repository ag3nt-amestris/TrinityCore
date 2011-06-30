/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
* Comment: No Waves atm and the doors spells are crazy...
*
* When your group enters the main room (the one after the bridge), you will notice a group of 3 Nerubians.
* When you engage them, 2 more groups like this one spawn behind the first one - it is important to pull the first group back,
* so you don't aggro all 3. Hadronox will be under you, fighting Nerubians.
*
* This is the timed gauntlet - waves of non-elite spiders
* will spawn from the 3 doors located a little above the main room, and will then head down to fight Hadronox. After clearing the
* main room, it is recommended to just stay in it, kill the occasional non-elites that will attack you instead of the boss, and wait for
* Hadronox to make his way to you. When Hadronox enters the main room, she will web the doors, and no more non-elites will spawn.
*/

#include "ScriptPCH.h"
#include "azjol_nerub.h"

enum Spells
{
    SPELL_ACID_CLOUD                              = 53400,
    H_SPELL_ACID_CLOUD                            = 59419,
    SPELL_LEECH_POISON                            = 53030,
    SPELL_LEECH_POISON_PCT                        = 53800,
    H_SPELL_LEECH_POISON                          = 59417,
    SPELL_PIERCE_ARMOR                            = 53418,
    SPELL_WEB_GRAB                                = 57731,
    H_SPELL_WEB_GRAB                              = 59421,
    SPELL_WEB_FRONT_DOORS                         = 53177,
    SPELL_WEB_SIDE_DOORS                          = 53185,
    // anubar crusher
    SPELL_FRENZY                                  = 53801,
    SPELL_SMASH                                   = 53318,
    H_SPELL_SMASH                                 = 59346
};

enum Creatures
{
    NPC_ANUBAR_CHAMPION                           = 29117,
    NPC_ANUBAR_CRYPT_FIEND                        = 29118
};

const Position HadronoxWaypoints[12] =
{
    {565.681458f, 513.247803f, 698.7f, 0.0f},
    {578.135559f, 512.468811f, 698.5f, 0.0f},
    {596.820007f, 510.811249f, 694.8f, 0.0f},
    {608.183777f, 513.395508f, 695.5f, 0.0f},
    {618.232422f, 525.414185f, 697.0f, 0.0f},
    {620.192932f, 539.329041f, 706.3f, 0.0f},
    {615.690979f, 552.474915f, 713.2f, 0.0f},
    {607.791870f, 566.636841f, 720.1f, 0.0f},
    {599.256104f, 580.134399f, 724.7f, 0.0f},
    {591.600220f, 589.028748f, 730.7f, 0.0f},
    {587.181580f, 596.026489f, 739.5f, 0.0f},
    {577.790588f, 583.640930f, 727.9f, 0.0f}
};

class boss_hadronox : public CreatureScript
{
public:
    boss_hadronox() : CreatureScript("boss_hadronox") { }

    struct boss_hadronoxAI : public ScriptedAI
    {
        boss_hadronoxAI(Creature* c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;

        uint32 uiAcidTimer;
        uint32 uiLeechTimer;
        uint32 uiPierceTimer;
        uint32 uiGrabTimer;
        uint32 uiStuckTimer;

        uint8 uiCount;
        bool bWP_reached;
        bool bMovementStarted;

        void Reset()
        {
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE); //
            me->SetReactState(REACT_PASSIVE);                        // remove later

            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 9.0f);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 9.0f);

            uiAcidTimer = urand(5*IN_MILLISECONDS, 10*IN_MILLISECONDS);
            uiLeechTimer = urand(7*IN_MILLISECONDS, 12*IN_MILLISECONDS);
            uiPierceTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            uiGrabTimer = urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS);
            uiStuckTimer = 3*IN_MILLISECONDS;

            uiCount = 0;
            bWP_reached = false;
            bMovementStarted = false;

            if (pInstance)
                pInstance->SetData(DATA_HADRONOX_EVENT, NOT_STARTED);
        }

        void KilledUnit(Unit* Victim)
        {
            if (Victim->HasAura(DUNGEON_MODE(SPELL_LEECH_POISON, H_SPELL_LEECH_POISON)) && me->isAlive())
                DoCast(me, SPELL_LEECH_POISON_PCT, true);
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE || id != uiCount)
                return;

            if (id < 11)
            {
                ++uiCount;
                bWP_reached = true;
            }
            else  // reached final wp
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE); //
                me->SetReactState(REACT_AGGRESSIVE);                        // remove later
            }
        }

        void JustDied(Unit* /*Killer*/)
        {
            if (pInstance)
                pInstance->SetData(DATA_HADRONOX_EVENT, DONE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (me->IsVisible() && !bMovementStarted)
            {
                bMovementStarted = true;
                bWP_reached = true;
            }

            if (bWP_reached)
            {
                bWP_reached = false;
                me->GetMotionMaster()->MovePoint(uiCount, HadronoxWaypoints[uiCount]);
            }

            if (!UpdateVictim()) 
                return;

            if (uiPierceTimer <= diff)
            {
                DoCast(me->getVictim(), SPELL_PIERCE_ARMOR);
                uiPierceTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else uiPierceTimer -= diff;

            if (uiAcidTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 60, false))
                    DoCast(pTarget, DUNGEON_MODE(SPELL_ACID_CLOUD, H_SPELL_ACID_CLOUD));

                uiAcidTimer = urand(15*IN_MILLISECONDS, 20*IN_MILLISECONDS);
            } else uiAcidTimer -= diff;

            if (uiLeechTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 20, false))
                    DoCast(pTarget, DUNGEON_MODE(SPELL_LEECH_POISON, H_SPELL_LEECH_POISON));

                uiLeechTimer = 10*IN_MILLISECONDS;
            } else uiLeechTimer -= diff;

            if (uiGrabTimer <= diff)
            {
                if (Unit* pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0))
                {
                    DoCast(me, DUNGEON_MODE(SPELL_WEB_GRAB, H_SPELL_WEB_GRAB));
                    DoCast(me, DUNGEON_MODE(SPELL_ACID_CLOUD, H_SPELL_ACID_CLOUD));
                }

                uiGrabTimer = urand(17*IN_MILLISECONDS, 23*IN_MILLISECONDS);
            } else uiGrabTimer -= diff;

            // workaround to unstuck boss, specially in fight versus npcs
            if (uiStuckTimer <= diff)
            {
                if (me->IsWithinCombatRange(me->getVictim(), 10.0f) && !me->IsWithinLOSInMap(me->getVictim()))
                    me->GetMotionMaster()->MoveJump(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 10.0f, 1.0f, 1.0f);
                uiStuckTimer = 3*IN_MILLISECONDS;
            } else uiStuckTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI *GetAI(Creature *creature) const
    {
        return new boss_hadronoxAI(creature);
    }
};

class npc_anubar_crusher : public CreatureScript
{
public:
    npc_anubar_crusher() : CreatureScript("npc_anubar_crusher") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_anubar_crusherAI(pCreature);
    }

    struct npc_anubar_crusherAI : public ScriptedAI
    {
        npc_anubar_crusherAI(Creature *c) : ScriptedAI(c) 
        {
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;

        uint32 uiSmashTimer;
        bool bFrenzy;

        void Reset()
        { 
            bFrenzy = false;
            uiSmashTimer = urand(7*IN_MILLISECONDS, 10*IN_MILLISECONDS);

            if (pInstance)
                pInstance->SetData(DATA_HADRONOX_EVENT, NOT_STARTED);

            if (Creature* pChampion = GetClosestCreatureWithEntry(me, NPC_ANUBAR_CHAMPION, 100.0f, false))
                pChampion->Respawn();
            if (Creature* pCryptFiend = GetClosestCreatureWithEntry(me, NPC_ANUBAR_CRYPT_FIEND, 100.0f, false))
                pCryptFiend->Respawn();
        }

        void EnterCombat(Unit* /*who*/) 
        {
            if (pInstance)
                pInstance->SetData(DATA_HADRONOX_EVENT, IN_PROGRESS);
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim()) 
                return;

            if (uiSmashTimer <= diff)
            {
                DoCast(me->getVictim(), DUNGEON_MODE(SPELL_SMASH, H_SPELL_SMASH));
                uiSmashTimer = urand(10*IN_MILLISECONDS, 15*IN_MILLISECONDS);
            } else uiSmashTimer -= diff;

            if (!bFrenzy && HealthBelowPct(30))
            {
                me->InterruptNonMeleeSpells(true);
                DoCast(me, SPELL_FRENZY, true);
                bFrenzy = true;
            }

            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_hadronox()
{
    new boss_hadronox();
    new npc_anubar_crusher();
}
