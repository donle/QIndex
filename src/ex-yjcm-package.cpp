#include "ex-yjcm-package.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "carditem.h"
#include "engine.h"
#include "ai.h"

class Jueqing: public TriggerSkill{
public:
    Jueqing():TriggerSkill("JueQing"){
        events << Predamage;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        int ex_damage = player->getMark("@struggle");
        DamageStruct damage = data.value<DamageStruct>();

        if((damage.card->inherits("Slash") || damage.card->inherits("Duel")) &&
            player->hasFlag("luoyi"))    ex_damage++;

        LogMessage log;
        log.type = "#Jueqing";
        log.from = player;
        log.to << damage.to;
        log.arg = QString::number(damage.damage+ex_damage);
        player->getRoom()->sendLog(log);
        player->getRoom()->playSkillEffect(objectName());

        player->getRoom()->loseHp(damage.to, damage.damage+ex_damage);
        return true;
    }
};

class Shangshi: public TriggerSkill{
public:
    Shangshi():TriggerSkill("ShangShi"){
        events << HpLost << Damaged << CardLost << PhaseChange << HpRecover;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Discard && player->getLostHp()>player->getHandcardNum()
            && player->getRoom()->askForSkillInvoke(player, objectName())){
            player->getRoom()->playSkillEffect(objectName());
            player->drawCards(player->getLostHp()-player->getHandcardNum());
        }
        return false;
    }
};

EX_YJCMPackage::EX_YJCMPackage():Package("EX_YJCM"){
    General *zhangchunhua = new General(this, "zhangchunhua", "wei", 3, false);
    zhangchunhua->addSkill(new Jueqing);
    zhangchunhua->addSkill(new Shangshi);
}

ADD_PACKAGE(EX_YJCM);
