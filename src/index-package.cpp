#include "index-package.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "client.h"
#include "carditem.h"
#include "serverplayer.h"
#include "clientplayer.h"
#include "settings.h"
#include "general.h"
#include "package.h"
#include "maneuvering.h"

class Jiyi: public PhaseChangeSkill{
public:
    Jiyi():PhaseChangeSkill("wanquanjiyi"){
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName()) && target->getHp() < target->getLostHp();
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if(target->getPhase() == Player::Play){
            if(room->askForSkillInvoke(target, objectName())){
               QList<ServerPlayer *> targets;
               foreach(ServerPlayer *p, room->getOtherPlayers(target)){
                   if(target->getTempData(p).isEmpty())
                       targets << p;
               }

               ServerPlayer *player = room->askForPlayerChosen(target, targets, objectName());
               QList<const Skill *> skills = player->getVisibleSkillList();
               target->setTempData(player, skills.length());
               foreach(const Skill *skill, skills){
                   if(skill->inherits("WeaponSkill") || skill->inherits("ArmorSkill"))
                       continue;
                   if(!player->hasSkill(skill->objectName()))
                       continue;

                   if(!skill->isLordSkill() && !skill->isLimitedSkill())
                       room->acquireSkill(target, skill->objectName());
                   if(skill->objectName() == "baiyi")
                       target->gainMark("@wings", 6);
               }
            }
        }

        return false;
    }
};

class Jiaohui: public TriggerSkill{
public:
    Jiaohui():TriggerSkill("yidongjiaohui"){
        frequency = Compulsory;
        events << Predamaged;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->getArmor() && target->getMark("qinggang") == 0;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Normal){
            LogMessage log;
            log.type = "#Jiaohui";
            log.from = player;
            player->getRoom()->sendLog(log);

            return true;
        }

        return false;
    }
};

class Yongchang: public TriggerSkill{
public:
    Yongchang():TriggerSkill("qiangzhiyongchang$"){
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasLordSkill("qiangzhiyongchang");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if(p->getKingdom() == "qingjiao")
                targets << p;
        }

        if(targets.length() == 0 || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        foreach(ServerPlayer *p, targets){
            QString choice = room->askForChoice(p, objectName(), "yes+no");
            if(choice == "yes"){
                room->loseHp(p);

                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
                return true;
            }
        }
        return false;
    }

};

class Huoyan: public TriggerSkill{
public:
    Huoyan():TriggerSkill("huoyanmoshu"){
        events << Predamage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();

        if(damage.nature != DamageStruct::Fire
           && room->askForSkillInvoke(player, objectName(), data) && room->askForDiscard(player, objectName(), 1, true))
            damage.nature = DamageStruct::Fire;

        if(damage.nature == DamageStruct::Fire){
            if(!player->hasFlag("fire") && room->askForSkillInvoke(player, "linghunranshao", data)){
                room->loseHp(player);
                damage.damage++;
                player->setFlags("fire");
            }
        }

        data = QVariant::fromValue(damage);
        return false;
    }
};

class Ranshao: public TriggerSkill{
public:
    Ranshao():TriggerSkill("linghunranshao"){
       events << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() == Player::Finish && player->hasFlag("fire"))
            player->setFlags("-fire");

        return false;
    }
};

WeishanCard::WeishanCard(){
    once = true;
    target_fixed = true;
}

void WeishanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->loseHp(source);
    room->setPlayerMark(source, "weishan_used", source->getLostHp());
}

class Weishan: public ZeroCardViewAsSkill{
public:
    Weishan():ZeroCardViewAsSkill("weishan"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("WeishanCard") && Slash::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        WeishanCard *card = new WeishanCard;
        card->setSkillName(objectName());
        return card;
    }
};

class WeishanClear: public TriggerSkill{
public:
    WeishanClear(): TriggerSkill("#weishan-clear"){
        events << Damage << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getMark("weishan_used") <= 0)
            return false;

        Room *room = player->getRoom();

        if(event == PhaseChange){
            if(player->getPhase() != Player::NotActive)
                return false;

            room->setPlayerMark(player, "weishan_used", 0);
        }
        else{
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.card->inherits("Slash"))
                player->drawCards(1);
        }
        return false;
    }
};

class Kaifa: public GameStartSkill{
public:
    Kaifa():GameStartSkill("chaonenglikaifa"){

    }

    virtual void onGameStart(ServerPlayer *player) const{
        Room *room = player->getRoom();

        QString choice = room->askForChoice(player, objectName(), "beicizhiren+routizaisheng");
        if(choice == "beicizhiren"){
            room->detachSkillFromPlayer(player, "routizaisheng");
            room->getThread()->removeTriggerSkill("routizaisheng");
        }
        else{
            room->detachSkillFromPlayer(player, "beicizhiren");
        }
        LogMessage log;
        log.type = "#KaifaDone";
        log.from = player;
        log.arg = choice;

        room->sendLog(log);
    }
};

BeiciCard::BeiciCard(){
    once = true;
}

bool BeiciCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getHandcardNum()>0;
}

void BeiciCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = source, *player = targets.first();

    LogMessage log;
    log.type = "#Beici";
    log.from = target;
    log.to << player;
    room->sendLog(log);

    int n = player->getHandcardNum();

    foreach(const Card *card, player->getHandcards()){
        room->moveCardTo(card, target, Player::Hand, false);
    }

    QList<int> cards_id;
    foreach(const Card *card, target->getHandcards()){
        cards_id << card->getEffectiveId();
    }

    while(n-- > 0){
        room->fillAG(cards_id, target);
        int card_id = room->askForAG(target, cards_id, false, "beicizhiren");
        room->moveCardTo(Sanguosha->getCard(card_id), player, Player::Hand, false);
        cards_id.removeOne(card_id);
        room->broadcastInvoke("clearAG");
    }

}

class Beici: public ZeroCardViewAsSkill{
public:
    Beici():ZeroCardViewAsSkill("beicizhiren"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("BeiciCard") && player->getHandcardNum() <= player->getHp();
    }

    virtual const Card *viewAs() const{
        Card *card = new BeiciCard;
        card->setSkillName(objectName());
        return card;
    }

};

class Zaisheng:public TriggerSkill{
public:
    Zaisheng():TriggerSkill("routizaisheng"){
        events << PhaseChange;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getPhase() == Player::Finish && player->isWounded()){
            LogMessage log;
            log.type = "#Zaisheng";
            log.from = player;
            room->sendLog(log);

            RecoverStruct recover;
            recover.recover = 1;
            recover.who = player;
            room->recover(player, recover);
        }
        return false;
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

class ImagineKiller: public TriggerSkill{
public:
    ImagineKiller(): TriggerSkill("huanxiangshashou"){
        events << SlashEffect << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == SlashEffect){
            LogMessage log;
            log.type = "#ImagineKiller2";
            log.from = player;
            room->sendLog(log);

            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(effect.nature != DamageStruct::Normal)
                effect.nature = DamageStruct::Normal;
            data = QVariant::fromValue(effect);
        }

        if(event == Predamaged){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.nature != DamageStruct::Normal && room->askForSkillInvoke(player, objectName(), data)){
                if(room->askForDiscard(player, objectName(), 1, true)){
                    LogMessage log;
                    log.type = "#ImagineKiller";
                    log.to << player;
                    log.from = damage.from;
                    log.arg = damage.card->objectName();
                    room->sendLog(log);

                    return true;
                }
            }
        }

        return false;
    }
};

ZeroSlash::ZeroSlash(Card::Suit suit, int number)
    :Slash(suit, number)
{
}

void ZeroSlash::setNature(DamageStruct::Nature nature){
    this->nature = nature;
}


class ZeroLevel: public FilterSkill{
public:
    ZeroLevel():FilterSkill("lingnenglizhe"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped() && to_select->getFilteredCard()->inherits("Weapon");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *c = card_item->getCard();
        Slash *slash = new ZeroSlash(c->getSuit(), c->getNumber());
        slash->setSkillName(objectName());
        slash->addSubcard(card_item->getCard());

        return slash;
    }
};

class Xixue: public TriggerSkill{
public:
    Xixue():TriggerSkill("xixueshashou"){
        events << HpRecover;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        Room *room = target->getRoom();
        ServerPlayer *qiusha = room->findPlayerBySkillName(objectName());
        return qiusha != NULL && qiusha != target;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        ServerPlayer *qiusha = room->findPlayerBySkillName(objectName());
        if(qiusha->getHandcardNum() == 0)
            return false;
        if(!room->askForSkillInvoke(qiusha, objectName(), data))
            return false;
        if(!room->askForDiscard(qiusha, objectName(), 1, true))
            return false;

        LogMessage log;
        log.type = "#Xixue";
        log.from = qiusha;
        log.to << player;
        room->sendLog(log);

        return true;
    }
};

class Cunzai: public ProhibitSkill{
public:
    Cunzai():ProhibitSkill("wucunzaigan"){
        frequency = Frequent;
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        return card->inherits("Slash") && !to->inMyAttackRange(from);
    }
};

class DianciPao: public TriggerSkill{
public:
    DianciPao():TriggerSkill("chaodiancipao"){
        events << SlashEffect << Damage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == Damage){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.nature == DamageStruct::Thunder){
                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                    if(player->distanceTo(p) <= 1 && p->getHandcardNum() > 0)
                        targets << p;
                }

                if(targets.length() == 0)
                    return false;

                if(!room->askForSkillInvoke(player, objectName(), data))
                    return false;

                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                int card_id = room->askForCardChosen(player, target, "h", objectName());
                room->moveCardTo(Sanguosha->getCard(card_id), player, Player::Hand, false);
            }
        }

        if(event == SlashEffect){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(effect.nature != DamageStruct::Thunder){
                effect.nature = DamageStruct::Thunder;
                data = QVariant::fromValue(effect);
            }
        }

        return false;
    }
};

class Pingzhang: public TriggerSkill{
public:
    Pingzhang():TriggerSkill("diancipingzhang"){
        events << Predamaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature == DamageStruct::Thunder){
            LogMessage log;
            log.type = "#Pingzhang";
            log.from = player;
            room->sendLog(log);

            return true;
        }

        if(player->distanceTo(damage.from) <= 1){
            if(!damage.card->inherits("Slash"))
                return false;
            LogMessage log;
            log.type = "#Pingzhang2";
            log.from = player;
            log.to << damage.from;
            room->sendLog(log);

            return true;
        }

        return false;
    }
};

class Xinling: public TriggerSkill{
public:
    Xinling():TriggerSkill("xinlingzhangwo"){
        events << PhaseChange << CardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->hasSkill(objectName()) && event == PhaseChange){
            if(player->getPhase() == Player::Finish && room->askForSkillInvoke(player, objectName(), data)){
                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                    if(player->distanceTo(p) <= 2)
                        targets << p;
                }
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                target->gainMark("@control");
            }
        }
        else if(player->getMark("@control") > 0){
            if(event == CardUsed){
                if(player->getPhase() == Player::NotActive)
                    return false;

                CardUseStruct use = data.value<CardUseStruct>();
                ServerPlayer *shifeng = room->findPlayerBySkillName(objectName());
                if(use.to.length() != 1 || use.to.at(0) == use.from || use.card->inherits("Nullification"))
                    return false;

                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                    if(use.card->inherits("Slash")){
                        if(player->inMyAttackRange(p))
                            targets << p;
                    }
                    else if(use.card->inherits("SupplyShortage") || use.card->inherits("Snatch")){
                        if(player->distanceTo(p) <= 1)
                            targets << p;
                    }
                    else
                        targets << p;
                }

                ServerPlayer *choice = room->askForPlayerChosen(shifeng, targets, objectName());
                use.to.clear();
                use.to << choice;

                data = QVariant::fromValue(use);
            }

            if(event == PhaseChange){
                if(player->getPhase() == Player::NotActive){
                    room->setPlayerMark(player, "@control", 0);
                }
            }
        }

        return false;
    }
};

class Yidong: public TriggerSkill{
public:
    Yidong():TriggerSkill("kongjianyidong"){
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(!effect.card->inherits("Slash") || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(spade|club):(.*)");
        judge.good = true;
        judge.who = player;
        room->judge(judge);

        if(judge.isGood()){
            LogMessage log;
            log.type = "#Yidong";
            log.from = player;
            log.to << player->getNextAlive();
            room->sendLog(log);

            CardEffectStruct new_effect = effect;
            new_effect.to = player->getNextAlive();

            room->cardEffect(new_effect);

            return true;
        }

        return false;
    }
};

class Shiliang: public TriggerSkill{
public:
    Shiliang(): TriggerSkill("shiliangcaozong"){
        events << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QList<const Card *> cards = player->getCards("h");
        if(cards.length() == 0)
            return false;

        DamageStruct damage = data.value<DamageStruct>();
        if(damage.from == NULL)
            return false;

        if(player->inMyAttackRange(damage.from) && room->askForSkillInvoke(player, objectName(), data)){
            JudgeStruct judge;
            if(player->getHp() > 3)
                judge.pattern = QRegExp("(.*):(heart):(.*)");
            else if(player->getHp() > 1)
                judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
            else
                judge.pattern = QRegExp("(.*):(heart|diamond|club):(.*)");

            judge.who = player;
            judge.good = true;
            room->judge(judge);

            if(judge.isGood()){
                room->askForDiscard(player, objectName(), 1, false, true);

                LogMessage log;
                log.type = "#ShiliangGood";
                log.from = player;
                log.to << damage.from;
                room->sendLog(log);

                damage.to = damage.from;
                room->damage(damage);
                return true;
            }
        }

        return false;
    }
};

class Tongxing: public TriggerSkill{
public:
    Tongxing():TriggerSkill("yifangtongxing$"){
        events << Predamaged;
    }

    virtual int getPrirority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if(p->getKingdom() == "darkness")
                targets << p;
        }

        if(targets.length() == 0 || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        foreach(ServerPlayer *p, targets){
            QString choice = room->askForChoice(p, objectName(), "yes+no");
            if(choice == "yes"){
                DamageStruct damage = data.value<DamageStruct>();
                damage.to = p;
                room->damage(damage);
                return true;
            }
        }

        return false;
    }
};

class Baocun: public OneCardViewAsSkill{
public:
    Baocun():OneCardViewAsSkill("dingwenbaocun"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if(Self->hasFlag("baocun_used")){
            return false;
        }

        return true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if(Self->hasFlag("baocun_used")){
            return false;
        }

        return true;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();
        bool valid = true;
        valid = !to_select->isEquipped() && !card->inherits("EquipCard")
                && !card->inherits("DelayedTrick") && !card->inherits("Jink") && !card->inherits("Nullification");

        if(!Slash::IsAvailable(Self))
            valid = valid && !card->inherits("Slash");
        if(!Self->isWounded())
            valid = valid && !card->inherits("Peach");
        if(!Analeptic::IsAvailable(Self))
            valid = valid && !card->inherits("Analeptic");

        return valid;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        Card *new_card = Sanguosha->cloneCard(card->objectName(), card->getSuit(), card->getNumber());
        Self->setFlags("baocun_used");
        new_card->setSkillName("dingwenbaocun");
        return new_card;
    }
};

class ShouhuSkill: public TriggerSkill{
public:
    ShouhuSkill():TriggerSkill("#shouhu-someone"){
        events << CardEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("@protect") > 0;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        Room *room = player->getRoom();

        if(effect.card->inherits("Dismantlement") || effect.card->inherits("Snatch")){
            LogMessage log;
            log.type = "#ShouhuInvoke";
            log.from = player;
            log.arg = effect.card->objectName();
            room->sendLog(log);

            return true;
        }

        return false;
    }
};

class Shouhu: public TriggerSkill{
public:
    Shouhu():TriggerSkill("shouhuzhishen"){
        events << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->hasSkill(objectName()) &&
           player->getPhase() == Player::Finish && room->askForSkillInvoke(player, objectName(), data)){
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());

            LogMessage log;
            log.type = "#Shouhu";
            log.to << target;
            log.from = player;
            room->sendLog(log);

            target->gainMark("@protect");
            room->acquireSkill(target, "shouhu");
        }

        if((player->getMark("@protect") > 0) && (player->getPhase() == Player::Start)){
            player->loseMark("@protect");
            room->detachSkillFromPlayer(player, "shouhu");
            player->loseSkill("shouhu");
        }

        return false;
    }
};

class Levelupper: public TriggerSkill{
public:
    Levelupper():TriggerSkill("huanxiangyushou"){
        events << PhaseChange << CardResponsed;
        frequency = Limited;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == CardResponsed){
            CardStar card = data.value<CardStar>();
            if(card->inherits("Slash"))
                player->setFlags("forbid_skill");
        }

        if(event == PhaseChange){
            if(player->getPhase() == Player::Start){
                player->setFlags("-forbid_skill");
            }
            else if(player->getPhase() == Player::Finish){
                if(!player->hasFlag("forbid_skill") &&
                   player->getSlashCount() == 0 &&
                   player->getMark("@upper") > 0){
                    QList<ServerPlayer *> targets, players = room->getOtherPlayers(player);
                    foreach(ServerPlayer *p, players){
                        if(!player->inMyAttackRange(p))
                            targets << p;
                    }
                    if(targets.length() == 0)
                        return false;

                    if(!room->askForSkillInvoke(player, objectName(), data))
                        return false;

                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());

                    LogMessage log;
                    log.type = "#LevelUp";
                    log.from = player;
                    log.to << target;
                    room->sendLog(log);

                    room->setPlayerProperty(player, "maxhp", player->getMaxHP()-1);
                    target->throwAllCards();
                    player->loseMark("@upper");
                }

                if(player->hasFlag("forbid_skill"))
                    player->setFlags("-forbid_skill");
            }
        }

        return false;
    }
};

class Upper: public TriggerSkill{
public:
    Upper():TriggerSkill("#konglishi-up"){
        events << DrawNCards << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill("huanxiangyushou") && target->getMark("@upper") <= 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == DrawNCards){
            data = data.toInt() + 1;
        }

        if(event == PhaseChange){
            if(player->getPhase() == Player::Discard)
                player->setXueyi(1);
        }

        return false;
    }
};

class StrongKonglishi: public SlashBuffSkill{
public:
   StrongKonglishi():SlashBuffSkill("dakonglishi"){

   }

    virtual bool buff(const SlashEffectStruct &effect) const{
        ServerPlayer *guangzi = effect.from;
        Room *room = guangzi->getRoom();
        if(room->getCurrent() != guangzi)
            return false;

        if(guangzi->getHp() > effect.to->getHp()){
            if(guangzi->askForSkillInvoke(objectName(), QVariant::fromValue(effect))){
                room->playSkillEffect(objectName());
                room->slashResult(effect, NULL);

                return true;
            }
        }
        return false;
    }
};

class Baopo: public TriggerSkill{
public:
    Baopo():TriggerSkill("nianlibaopo"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();

        if(!damage.card->inherits("Slash") || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        CardUseStruct use;
        use.card = damage.card;
        use.from = damage.from;
        use.to << damage.to->getNextAlive();
        room->useCard(use, false);

        return false;
    }
};

class Zuobiao: public TriggerSkill{
public:
    Zuobiao():TriggerSkill("zuobiaoyidong"){
        events << CardEffected;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getCards("he").length() == 0)
            return false;

        Room *room = player->getRoom();
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.multiple || effect.from == NULL || effect.from == effect.to)
            return false;

        if(effect.card->inherits("Slash") || effect.card->inherits("TrickCard")){
            if(!room->askForSkillInvoke(player, objectName(), data))
                return false;

            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
            judge.who = player;
            judge.good = true;
            room->judge(judge);

            if(judge.isGood()){
                LogMessage log;
                log.type = "#ZuobiaoGood";
                log.from = player;
                log.to << effect.from;
                room->sendLog(log);

                player->addMark("exchanged");

                ServerPlayer *exchange = effect.from;
                effect.from = effect.to;
                effect.to = exchange;
                data = QVariant::fromValue(effect);
            }
        }

        return false;
    }
};

class Yizhi: public TriggerSkill{
public:
    Yizhi():TriggerSkill("jingshenyizhi"){
        events << SlashEffect;
        frequency = Control;
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("exchanged") > 4;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        if(player->hasFlag("yizhi_effect"))
            return false;

        Room *room = player->getRoom();
        LogMessage log;
        log.type = "#Yizhi";
        log.from = player;
        room->sendLog(log);


        JudgeStruct judge;
        judge.pattern = QRegExp("(.*):(club|spade):(.*)");
        judge.who = player;
        judge.good = true;
        room->judge(judge);

        if(judge.isGood()){
            LogMessage log;
            log.type = "#YizhiEffect";
            log.from = player;
            room->sendLog(log);

            player->setFlags("yizhi_effect");
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to = effect.from;
            room->slashEffect(effect);
            player->setFlags("-yizhi_effect");

            return true;
        }

        return false;
    }

};

class Tianshi: public TriggerSkill{
public:
    Tianshi():TriggerSkill("renzaotianshi"){
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        LogMessage log;
        log.type = "#TianshiEffect";
        log.from = player;
        room->sendLog(log);

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
        if(player->getHandcardNum() > 0){
            const Card *card = room->askForCard(player, ".", "@angel-card", false);
            room->moveCardTo(card, target, Player::Hand, false);
        }
        else
            target->drawCards(1);

        if(player->getMark("buming") > 0){
            LogMessage log;
            log.type = "#TianshiEx";
            log.from = player;
            log.to << target;
            room->sendLog(log);

            if(player->getHandcardNum() > 0){
                const Card *card2 = room->askForCard(player, ".", "@angel-card", false);
                room->moveCardTo(card2, target, Player::Hand, false);
            }
            else
                target->drawCards(1);
        }
        else
            player->drawCards(1);
        return false;
    }
};

class BumingSelect:public TriggerSkill{
public:
    BumingSelect():TriggerSkill("#buming"){
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill("zhengtibuming");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QString role;
        if(player->getMark("buming") == 1)
            role = "rebel";
        else if(player->getMark("buming") == 2)
            role = "loyalist";
        else if(player->getMark("buming") == 3)
            role = "renegade";

        if(role.length() == 0)
            return false;

        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if(p->getRole() != player->getRole())
                targets << p;
        }
        targets.removeOne(room->getLord());

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());

        LogMessage log;
        log.type = "#Zhengti";
        log.from = player;
        log.to << target;
        room->sendLog(log);

        room->setPlayerProperty(target, "role", role);

        return false;
    }
};

class Buming:public GameStartSkill{
public:
    Buming():GameStartSkill("zhengtibuming"){

    }

    virtual int getPriority() const{
        return -1;
    }

    virtual void onGameStart(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if(player->isLord() || ServerInfo.GameMode == "02_1v1")
            return;

        if(player->isLord() || !room->askForSkillInvoke(player, objectName()))
            return;

        QString choice = room->askForChoice(player, objectName(), "loyalist+rebel+renegade");
        if(player->getRole() == choice)
            return;

        if(choice == "loyalist")
            player->setMark("buming", 1);
        else if(choice == "rebel")
                player->setMark("buming", 2);
        else if(choice == "renegade")
                player->setMark("buming", 3);
        QVariant data = QVariant::fromValue(choice);
        player->setProperty("role", data);

        LogMessage log;
        log.type = "#BumingSelect";
        log.from = player;
        room->sendLog(log);
    }
};

class Cilijuji: public FilterSkill{
public:
    Cilijuji():FilterSkill("cilijujipao"){
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("Slash");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *c = card_item->getCard();
        Slash *slash = new ZeroSlash(c->getSuit(), c->getNumber());
        slash->setSkillName(objectName());
        slash->addSubcard(card_item->getCard());

        return slash;
    }

};

class CiliSlash: public TriggerSkill{
public:
    CiliSlash():TriggerSkill("#cilislash"){
        events << SlashEffect << SlashHit << SlashMissed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(event == SlashEffect){
            effect.nature = DamageStruct::Thunder;
            data = QVariant::fromValue(effect);

            effect.to->addMark("qinggang");
        }

        return false;
    }
};

class Xuji: public TriggerSkill{
public:
    Xuji():TriggerSkill("cilixuji"){
        events << SlashEffect;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getCards("e").length() == 0 || !room->askForSkillInvoke(player, objectName(), data))
            return false;
        int card_id = room->askForCardChosen(player, player, "e", objectName());
        if(card_id == -1)
            return false;
        room->throwCard(card_id, true);

        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        effect.ex_damage = 1;
        data = QVariant::fromValue(effect);

        return false;
    }
};

JinxingCard::JinxingCard(){
    once = true;
}

bool JinxingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

void JinxingCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    ServerPlayer *to = targets.at(0);
    int n = to->getCards("he").length();
    to->throwAllEquips();
    to->throwAllHandCards();
    to->drawCards(n);
    to->addMark("golden");
}

class Jinxing: public OneCardViewAsSkill{
public:
    Jinxing():OneCardViewAsSkill("jinxingguangxian"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("Weapon");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        JinxingCard *card = new JinxingCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class Gun: public TriggerSkill{
public:
    Gun():TriggerSkill("kewutuolizhiqiang"){
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();


        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if(p->getMark("golden") > 0)
                targets << p;
        }
        if(targets.length() == 0)
            return false;

        if(!room->askForSkillInvoke(player, objectName(), data))
            return false;

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
        if(target->getHandcardNum() > 0){
            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart|club|diamond):(.*)");
            judge.who = target;
            judge.good = true;
            judge.reason = objectName();
            room->judge(judge);

            if(judge.isGood()){
                LogMessage log;
                log.type = "#GunEffect";
                log.from = player;
                log.to << target;
                room->loseHp(target, target->getHp());
            }
        }

        return false;
    }
};

class Shixiang: public TriggerSkill{
public:
    Shixiang():TriggerSkill("yourenshixiang"){
        events << SlashEffect;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        SlashEffectStruct slash_effect = data.value<SlashEffectStruct>();
        ServerPlayer *target = slash_effect.to;

        if(target->hasFlag("triggered")){
            target->setFlags("-triggered");
            return false;
        }

        ServerPlayer *prior = NULL, *next = target->getNextAlive();

        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if(p->getNextAlive() == target){
                prior = p;
                break;
            }
        }


        if(prior != NULL){
            slash_effect.to = prior;
            slash_effect.to->setFlags("triggered");
            room->slashEffect(slash_effect);
        }

        if(prior != next){
            slash_effect.to = next;
            slash_effect.to->setFlags("triggered");
            room->slashEffect(slash_effect);
        }

        LogMessage log;
        log.type = "#Shixiang";
        log.from = player;
        log.to << target;
        log.arg = prior->getGeneralName();
        log.arg2 = next->getGeneralName();
        room->sendLog(log);

        return false;
    }
};

class Five: public OneCardViewAsSkill{
public:
    Five():OneCardViewAsSkill("diwuyuansu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("five_used") < 2;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Duel *duel = new Duel(card->getSuit(), card->getNumber());
        duel->addSubcard(card);
        duel->setSkillName(objectName());
        return duel;
    }
};

class Wuzhi: public MasochismSkill{
public:
    Wuzhi():MasochismSkill("weiyuanwuzhi"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        if(!room->askForSkillInvoke(target, objectName()))
            return;

        QString choice = room->askForChoice(target, objectName(), "getmark+exnihilo");
        if(choice == "getmark")
            target->gainMark("@wings", 2);
        else{
            if(target->getMark("@wings") < 2)
                return;

            ExNihilo *_ex = new ExNihilo(Card::NoSuit, 0);
            _ex->setSkillName(objectName());
            target->loseMark("@wings", 2);
            CardUseStruct use;
            use.card = _ex;
            use.from = target;
            use.to << target;
            room->useCard(use);
        }
    }
};

class BaiyiStart: public GameStartSkill{
public:
    BaiyiStart():GameStartSkill("#baiyi"){
        frequency = Compulsory;
    }

    virtual void onGameStart(ServerPlayer *player) const{
        player->gainMark("@wings", 6);
    }
};

BaiyiCard::BaiyiCard(){
    once = true;
}

bool BaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.isEmpty())
        return !Self->inMyAttackRange(to_select) &&
                !Self->isProhibited(to_select, Sanguosha->getCard(subcards.first()));
    else
        return false;
}

void BaiyiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    CardUseStruct use;
    use.card = Sanguosha->getCard(subcards.first());
    use.from = source;
    use.to << targets.first();
    room->useCard(use);
}

class Baiyi: public OneCardViewAsSkill{
public:
    Baiyi(): OneCardViewAsSkill("baiyi"){

    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "jink" && player->getMark("@wings") > 0;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player) && player->getMark("@wings") > 0;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        if(ClientInstance->getStatus() == Client::Responsing)
            return true;

        return to_select->getFilteredCard()->inherits("Slash");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *c = card_item->getFilteredCard();

        if(c->inherits("Slash")){
            BaiyiCard *card = new BaiyiCard;
            card->setSkillName(objectName());
            card->addSubcard(c);
            return card;
        }

        Jink *jink = new Jink(c->getSuit(), c->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(c);
        return jink;
    }
};

class BaiyiUse: public TriggerSkill{
public:
    BaiyiUse():TriggerSkill("#baiyi-used"){
        events << CardResponsed << CardUsed;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == CardResponsed){
            CardStar card = data.value<CardStar>();
            if(card->inherits("Jink") && card->getSkillName() == "baiyi")
                player->loseMark("@wings");
        }

        if(event == CardUsed && player->getPhase() == Player::Play){
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->inherits("Slash") && use.card->getSkillName() == "baiyi")
                player->loseMark("@wings");
        }

        return false;
    }
};

class Yuanzi: public TriggerSkill{
public:
    Yuanzi():TriggerSkill("yuanzibenghuai"){
        events << CardEffect;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        Room *room = target->getRoom();
        ServerPlayer *chenli = room->findPlayerBySkillName(objectName());
        if(chenli == NULL)
            return false;
        return chenli != target;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        ServerPlayer *chenli = room->findPlayerBySkillName(objectName());
        if(chenli == player || chenli->distanceTo(player) > 2)
            return false;

        CardEffectStruct effect = data.value<CardEffectStruct>();

        if(effect.to->hasFlag("atom_effect")){
            player->setFlags("-atom_effect");
            return true;
        }
        if(effect.to->hasFlag("atom_lose")){
            player->setFlags("-atom_lose");
            return false;
        }

        if((effect.multitargets.contains(chenli) || effect.to == chenli) &&
           effect.card->inherits("TrickCard")){
            if(!room->askForSkillInvoke(chenli, objectName(), data)){
                foreach(ServerPlayer *p, effect.multitargets){
                     p->setFlags("atom_lose");
                }
                return false;
            }

            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(spade|heart):(.*)");
            judge.who = player;
            judge.good = true;
            room->judge(judge);

            foreach(ServerPlayer *p, effect.multitargets){
                if(judge.isGood())
                    p->setFlags("atom_effect");
                else
                    p->setFlags("atom_lose");
            }

            if(judge.isGood()){
                LogMessage log;
                log.type = "#YuanziEffect";
                log.from = chenli;
                log.to << player;
                room->sendLog(log);

                return true;
            }
        }

        return false;
    }
};

class Lizipao: public TriggerSkill{
public:
    Lizipao():TriggerSkill("liziboxinggaosupao"){
        events << CardResponsed << Predamage;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == CardResponsed){
            CardStar card = data.value<CardStar>();
            if(card->inherits("Jink"))
                player->gainMark("@atom");
        }

        if(event == Predamage && player->getPhase() == Player::Play){
            if(player->getMark("@atom") <= 0 || !room->askForSkillInvoke(player, objectName(), data))
                return false;

            int n = player->getMark("@atom");
            DamageStruct damage = data.value<DamageStruct>();

            LogMessage log;
            log.type = "#Lizipao";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(n);
            room->sendLog(log);

            damage.damage += n;
            data = QVariant::fromValue(damage);
            player->loseAllMarks("@atom");
        }

        return false;
    }
};

class DanqiCollect: public TriggerSkill{
public:
    DanqiCollect():TriggerSkill("#danqi-collect"){
        events << CardDiscarded;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Discard)
            return false;
        ServerPlayer *zuiai = player->getRoom()->findPlayerBySkillName(objectName());
        if(zuiai == NULL)
            return false;

        const Card *card = data.value<CardStar>();
        if(card->subcardsLength() == 0)
            return false;

        if(!card->isVirtualCard() && card->getSuit() != Card::Diamond)
            zuiai->addToPile("nitro", card->getEffectiveId());
        else{
            foreach(int card_id, card->getSubcards()){
                const Card *add = Sanguosha->getCard(card_id);
                if(add->getSuit() != Card::Diamond && add->getSuit() != Card::Heart)
                    zuiai->addToPile("nitro", card_id);
            }
        }

        return false;
    }
};

class Danqi: public TriggerSkill{
public:
    Danqi():TriggerSkill("danqizhuangjia"){
        events << Predamage << Predamaged;
    }

    void throw_danqi_card(Room *room, ServerPlayer *player, QList<int> &danqi_pile) const{

        room->fillAG(danqi_pile);
        for(int i = 0; i < 2; i++){
            int card_id = room->askForAG(player, danqi_pile, false, objectName());
            room->takeAG(NULL, card_id);
            danqi_pile.removeOne(card_id);
        }

        player->clearPrivatePiles();
        foreach(int id, danqi_pile)
            player->addToPile("nitro", id);

        room->broadcastInvoke("clearAG");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        QList<int> danqi_pile = player->getPile("nitro");
        if(danqi_pile.length() < 2)
            return false;

        if(!room->askForSkillInvoke(player, objectName(), data))
            return false;

        if(event == Predamaged){
            LogMessage log;
            log.type = "#DanqiDefense";
            log.from = player;
            room->sendLog(log);

            throw_danqi_card(room, player, danqi_pile);

            damage.damage--;
            data = QVariant::fromValue(damage);
        }
        if(event == Predamage){
            LogMessage log;
            log.type = "#DanqiOffense";
            log.from = player;
            log.to << damage.to;
            room->sendLog(log);

            throw_danqi_card(room, player, danqi_pile);

            damage.damage++;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

ShenyinCard::ShenyinCard(){
    target_fixed = true;
    once = true;
}

void ShenyinCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    const Card *card = room->askForExchange(source, "shenyin", 1);

    source->addToPile("guess", card->getEffectiveId(), false);
}

class Shenyin: public ZeroCardViewAsSkill{
public:
    Shenyin():ZeroCardViewAsSkill("shenyin"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !Self->hasUsed("ShenyinCard");
    }

    virtual const Card *viewAs() const{
        return new ShenyinCard;
    }
};

class Shushi: public TriggerSkill{
public:
    Shushi():TriggerSkill("shushi"){
        events << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getPhase() == Player::Finish && target->getPile("guess").length() > 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        if(!room->askForSkillInvoke(player, objectName()))
            return false;

        QList<int> cards_id = player->getPile("guess");
        room->fillAG(cards_id);
        int card_id = room->askForAG(player, cards_id, false, "shushi");
        room->broadcastInvoke("clearAG");

        const Card *card = Sanguosha->getCard(card_id);
        Slash *slash = new FireSlash(card->getSuit(), card->getNumber());
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *t, room->getOtherPlayers(player)){
            if(!t->isProhibited(player, slash))
                targets << t;
        }

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
        QString choice = room->askForChoice(target, objectName(), "yes+no");

        if(choice == "no"){
            LogMessage log;
            log.type = "#ShenyinSlash";
            log.from = player;
            log.to << target;
            log.arg = card->objectName();
            room->sendLog(log);

            CardUseStruct use;
            slash->setSkillName(objectName());
            use.card = slash;
            use.from = player;
            use.to << target;
            room->useCard(use, false);

            room->throwCard(card_id);
        }
        else{
            room->showCard(player, card_id, target);

            LogMessage log;
            log.type = "#ShenyinCheck";
            log.from = player;
            log.to << target;
            log.arg = card->objectName();
            room->sendLog(log);

            if(card->inherits("Slash")){
                DamageStruct damage;
                damage.card = card;
                damage.from = player;
                damage.to = target;
                damage.nature = DamageStruct::Fire;

                room->damage(damage);

                room->throwCard(card_id);
            }
            else{
                room->moveCardTo(card, target, Player::Hand, false);
            }
        }

        return false;
    }
};

ShendianCard::ShendianCard(){
    once = true;
}

bool ShendianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getMark("@temple") <= 0;
}

void ShendianCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->gainMark("@temple");
    target->setFlags("shendian_effect");
}

class Shendian: public ZeroCardViewAsSkill{
public:
    Shendian():ZeroCardViewAsSkill("shouhushendian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !Self->hasUsed("ShendianCard");
    }

    virtual const Card *viewAs() const{
        return new ShendianCard;
    }
};

class ShendianEffect: public TriggerSkill{
public:
    ShendianEffect():TriggerSkill("#shendian"){
        events << CardDiscarded << PhaseChange;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("@temple") > 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == PhaseChange){
            if(player->getPhase() == Player::Start)
                player->setFlags("shendian_effect");
            if(player->getPhase() == Player::Finish){
                if(player->hasFlag("shendian_effect")){
                    LogMessage log;
                    log.type = "#ShendianLose";
                    log.from = player;
                    room->sendLog(log);

                    player->loseAllMarks("@temple");
                }
                else{
                    player->gainMark("@temple");
                    if(player->getMark("@temple") > 2){
                        player->loseAllMarks("@temple");

                        LogMessage log;
                        log.type = "#ShendianEffect";
                        log.from = player;
                        room->sendLog(log);

                        RecoverStruct recover;
                        recover.who = player;
                        recover.recover = qMin(player->getLostHp(), 2);
                        room->recover(player, recover);
                    }
                }

            }
        }

        if(event == CardDiscarded && player->getPhase() == Player::Discard){
            player->setFlags("-shendian_effect");
        }

        return false;
    }
};

class Jitiyizhi: public TriggerSkill{
public:
    Jitiyizhi():TriggerSkill("jitiyizhi"){
        events << AskForPeachesDone;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(player->getCards("e").length() == 0 || player->getHp() > 0)
            return false;

        if(!room->askForSkillInvoke(player, objectName(), data))
            return false;

        int n = player->getCardCount(true);
        player->throwAllEquips();
        player->throwAllCards();

        RecoverStruct recover;
        recover.who = player;
        recover.recover = qMin(3, n);
        room->recover(player, recover);

        return false;
    }
};

class Shuimian: public TriggerSkill{
public:
    Shuimian():TriggerSkill("lengdongshuimian"){
        events << PhaseChange << Predamaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == PhaseChange){
            if(player->getPhase() == Player::Finish){
                if(!player->isWounded() || !room->askForSkillInvoke(player, objectName(), data))
                    return false;

                room->loseHp(player);
                if(player->isDead())
                    return false;

                player->drawCards(4);
                player->turnOver();
                if(player->getArmor() != NULL)
                    room->throwCard(player->getArmor()->getEffectiveId(), true);
                player->addMark("freeze");
            }

            if(player->getPhase() == Player::Start){
                if(player->getMark("freeze") > 0)
                    player->removeMark("freeze");
            }
        }
        if(event == Predamaged){
            if(player->getMark("freeze") > 0){
                LogMessage log;
                log.type = "#Sleeping";
                log.from = player;
                room->sendLog(log);

                return true;
            }
        }

        return false;
    }
};

class Jiejing: public TriggerSkill{
public:
    Jiejing():TriggerSkill("nenglitijiejing"){
        events << Predamaged << AskForPeachesDone << PhaseChange;
        frequency = Limited;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == Predamaged){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.from)
                damage.from->setFlags("jiejing_target");
        }
        if(event == AskForPeachesDone){
            if(player->getMark("jiejing_final") > 0)
                return true;
            if(player->getMark("@lens") == 0)
                return false;
            if(player->getHp() > 0)
                return false;
            if(!room->askForSkillInvoke(player, objectName(), data))
                return false;

            player->loseAllMarks("@lens");
            QList<ServerPlayer *> others = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, others){
                if(p->getMark("@lens") > 0 || p->hasFlag("jiejing_target")){
                    const General *general = p->getGeneral();
                    QList<const Skill *> skills = general->findChildren<const Skill *>();
                    foreach(const Skill *skill, skills){
                        if(!skill->isLordSkill())
                            room->acquireSkill(player, skill->objectName());
                    }
                    p->loseAllMarks("@lens");
                }
            }

            player->addMark("jiejing_final");
            player->drawCards(2);
            room->setCurrent(player);
            player->play();
        }

        if(event == PhaseChange
           && player->getMark("jiejing_final")
           && player->getHp() <= 0
           && player->getPhase() == Player::NotActive){
            room->killPlayer(player);
        }

        return false;
    }
};

class JiejingTrigger: public TriggerSkill{
public:
    JiejingTrigger(): TriggerSkill("#jiejing"){
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.from != NULL && damage.from->getMark("@lens") <= 0)
            damage.from->gainMark("@lens");

        return false;
    }
};

HunluanCard::HunluanCard(){
    once = true;
    target_fixed = true;
}

void HunluanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    if(room->askForDiscard(source, "nenglihunluan", 1, true))
        room->setPlayerFlag(source, "hunluan_slash");
}

class HunluanViewAsSkill: public ZeroCardViewAsSkill{
public:
    HunluanViewAsSkill(): ZeroCardViewAsSkill("hunluan"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("HunluanCard") && player->getWeapon()
                && Slash::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        HunluanCard *card = new HunluanCard;
        card->setSkillName("nenglihunluan");
        return card;
    }
};

class Hunluan: public TriggerSkill{
public:
    Hunluan():TriggerSkill("nenglihunluan"){
        events << SlashEffect;
        view_as_skill = new HunluanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(!player->getWeapon() || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        JudgeStruct judge;
        judge.pattern = QRegExp("(Slash|Jink|Peach|Analeptic|Shit):(.*):(.*)");
        judge.who = player;
        judge.good = true;
        room->judge(judge);

        if(judge.isGood()){
            LogMessage log;
            log.type = "#HunluanSuccess";
            log.from = player;
            room->sendLog(log);

            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.ex_damage = 1;
            data = QVariant::fromValue(effect);
        }

        return false;
    }
};

class Wangzuo: public TriggerSkill{
public:
    Wangzuo():TriggerSkill("beiouwangzuo"){
        events << CardEffected;
        frequency = Frequent;
    }

    virtual int getPriortiy() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardEffectStruct effect = data.value<CardEffectStruct>();

        if(effect.from == effect.to || effect.from == NULL || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        bool is_ag = false;
        ServerPlayer *four_area = NULL;
        QList<int> card_ids;
        QVariantList ag_list;
        if(effect.card->inherits("AmazingGrace")){
            ag_list = room->getTag("AmazingGrace").toList();
            foreach(QVariant c_id, ag_list)
                card_ids << c_id.toInt();

            is_ag = true;
        }
        else if(effect.card->inherits("FourArea")){
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if(player->getTempData(p).first() == effect.card->getEffectiveId()){
                    if(player->getMark("second_target") > 0
                       && player->getPhase() != Player::Play){
                        player->removeMark("second_target");
                        break;
                    }

                    room->broadcastInvoke("clearAG");
                    four_area = p;
                    player->clearTempData(true);
                    break;
                }
            }
        }

        QList<int> cards_id = room->getNCards(2);
        room->fillAG(cards_id, player);
        int card_id = room->askForAG(player, cards_id, false, objectName());
        cards_id.removeOne(card_id);
        room->takeAG(player, card_id);
        room->broadcastInvoke("clearAG");

        room->doGuanxing(player, cards_id, true, true);

        if(is_ag)
            room->fillAG(card_ids);
        else if(four_area != NULL)
            room->showAllCards(four_area, player);

        return false;
    }
};

class Unknown: public TriggerSkill{
public:
    Unknown():TriggerSkill("nenglibuming"){
        events << Damage;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        if(!damage.card->inherits("Slash") || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        QList<int> cards_id = room->getNCards(qMin(room->getAlivePlayers().length(), 5));
        room->fillAG(cards_id, player);
        int n = damage.damage;
        for(int i = 0; i < n; i++){
            int card_id = room->askForAG(player, cards_id, false, objectName());
            cards_id.removeOne(card_id);
            room->takeAG(player, card_id);
        }
        room->broadcastInvoke("clearAG");

        if(!cards_id.isEmpty())
            room->doGuanxing(player, cards_id, true, true);

        room->askForDiscard(player, objectName(), n);

        return false;
    }
};

class Toushi: public TriggerSkill{
public:
    Toushi():TriggerSkill("qianglitoushi"){
        events << PhaseChange;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getPhase() != Player::Draw || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        QList<int> card_ids = room->getNCards(2);
        int n = 2;
        room->fillAG(card_ids, player);
        while(!card_ids.isEmpty()){
            int card_id = room->askForAG(player, card_ids, true, objectName());
            if(card_id == -1)
                break;
            if(!card_ids.contains(card_id))
                continue;

            card_ids.removeOne(card_id);
            room->takeAG(player, card_id);
            n--;
        }
        room->broadcastInvoke("clearAG");

        if(!card_ids.isEmpty())
            room->doGuanxing(player, card_ids, true, true);

        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            if(p->getHandcardNum() > 0)
                targets << p;

        for(int i = 0; i < n; i++){
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
            targets.removeOne(target);

            QList<int> target_card;
            foreach(const Card *card, target->getHandcards()){
                target_card << card->getEffectiveId();
            }

            room->fillAG(target_card, player);
            int target_card_id = room->askForAG(player, target_card, false, objectName());
            room->moveCardTo(Sanguosha->getCard(target_card_id), player, Player::Hand, false);
            room->broadcastInvoke("clearAG");
            target->drawCards(1);
        }

        return true;
    }
};

BaoqiangCard::BaoqiangCard(){
    target_fixed = true;
    setObjectName("baoqiang");
}

void BaoqiangCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->throwCard(subcards.first(), true);
    source->gainMark("@wan", 2);
}

class Baoqiang: public FilterSkill{
public:
    Baoqiang():FilterSkill("danqibaoqiang"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();
        return card->inherits("Armor") || card->inherits("DefensiveHorse");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        BaoqiangCard *card = new BaoqiangCard;
        card->addSubcard(card_item->getCard());
        return card;
    }
};

class Diejia: public TriggerSkill{
public:
    Diejia():TriggerSkill("gongjidiejia"){
        events << SlashEffect << Predamage << Death;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == Predamage){
            DamageStruct damage = data.value<DamageStruct>();
            if(room->askForSkillInvoke(player, objectName(), data)){
                damage.to->gainMark("@add", damage.damage);

                room->getThread()->trigger(DamageComplete, player, data);
                return true;
            }
            else{
                if(damage.to->getMark("@add") > 0){
                    damage.damage += damage.to->getMark("@add");
                    data = QVariant::fromValue(damage);
                    damage.to->loseAllMarks("@add");
                }
            }
        }else if(event == SlashEffect){
            if(player->getMark("@wan") <= 0 || !room->askForSkillInvoke(player, "danqibaoqiang", data))
                return false;

            player->loseMark("@wan");
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.ex_damage = 1;

            data = QVariant::fromValue(effect);
        }else{
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if(p->getMark("@add") > 0)
                    p->loseAllMarks("@add");
        }
        return false;
    }
};

ZhuijiCard::ZhuijiCard(){
    once = true;
}

void ZhuijiCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->addMark("control_follow");
    effect.to->gainMark("@follow");
    Room *room = effect.to->getRoom();

    LogMessage log;
    log.type = "#ZhuijiAt";
    log.to << effect.to;
    room->sendLog(log);
}

class Zhuiji: public ZeroCardViewAsSkill{
public:
    Zhuiji():ZeroCardViewAsSkill("nenglizhuiji"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ZhuijiCard");
    }

    virtual const Card *viewAs() const{
        return new ZhuijiCard;
    }
};

class ZhuijiOn: public TriggerSkill{
public:
    ZhuijiOn():TriggerSkill("#zhuiji"){
        events << CardEffected << PhaseChange;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->getSkillName() != "zhuiji")
                return false;

            QList<const Skill *> skills = player->getVisibleSkillList();
            foreach(const Skill *skill, skills){
                if(skill->inherits("WeaponSkill") || skill->inherits("ArmorSkill"))
                    continue;

                room->detachSkillFromPlayer(player, skill->objectName());
                player->setFlags(skill->objectName());

                if(skill->isTriggerSkill())
                    room->getThread()->removeTriggerSkill(skill->objectName());
            }
        }

        if(event == PhaseChange && player->getMark("@follow") > 0 && player->getPhase() == Player::NotActive){
            QStringList skill_names = player->getFlags().split("+");
            foreach(QString skill, skill_names){
                room->acquireSkill(player, skill);
            }
            player->loseMark("@follow");
        }

        return false;
    }
};

class Baozou: public TriggerSkill{
public:
    Baozou():TriggerSkill("baozouzhuangtai"){
        events << CardUsed;
        frequency = Control;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("control_follow") > 2 && target->isWounded();
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->getSkillName() != "zhuiji")
            return false;

        LogMessage log;
        log.type = "#BaozouEffect";
        log.from = player;
        room->sendLog(log);

        room->loseHp(player, player->getLostHp());

        return false;
    }
};

DingguiCard::DingguiCard(){
    once = true;
}

bool DingguiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

bool DingguiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < 2;
}

void DingguiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *from = targets.at(0), *to = targets.at(1);
    int value1 = from->distanceTo(to), value2 = to->distanceTo(from);
    source->setTempData(from, value1, false);
    source->setTempData(to, value2, false);
    from->addMark("from");
    to->addMark("to");

    if(source->getHp()>2){
        room->setFixedDistance(from, to, 1);
        room->setFixedDistance(to, from, 1);
    }
    else{
        room->setFixedDistance(from, to, 999);
        room->setFixedDistance(to, from, 999);
    }
}

class Dinggui: public ZeroCardViewAsSkill{
public:
    Dinggui():ZeroCardViewAsSkill("xinlidinggui"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("DingguiCard");
    }

    virtual const Card *viewAs() const{
        return new DingguiCard;
    }
};

class DingguiClear: public TriggerSkill{
public:
    DingguiClear():TriggerSkill("#dinggui-clear"){
        events << PhaseChange;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getPhase() != Player::Start)
            return false;

        int value1 = 0, value2 = 0;
        ServerPlayer *from = NULL, *to = NULL;
        foreach(ServerPlayer *p, room->getAllPlayers()){
            QList<int> distances = player->getTempData(p);
            if(distances.length() > 0){
                if(p->getMark("from") > 0){
                    from = p;
                    value1 = distances.first();
                    p->removeMark("from");
                }

                if(p->getMark("to") > 0){
                    to = p;
                    value2 = distances.first();
                    p->removeMark("to");
                }
            }
        }

        if(from && to){
            room->setFixedDistance(from, to, value1);
            room->setFixedDistance(to, from, value2);
        }
        player->clearTempData(true);
        return false;
    }
};

ShufuCard::ShufuCard(){
    once = true;
}

void ShufuCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", "mofashufu");
    const Card *card = Sanguosha->getCard(card_id);
    bool is_public = room->getCardPlace(card_id) != Player::Hand;
    room->moveCardTo(card, effect.from, Player::Hand, is_public ? true : false);

    RecoverStruct recover;
    recover.who = effect.from;
    room->recover(effect.from, recover);
}

class Shufu: public OneCardViewAsSkill{
public:
    Shufu():OneCardViewAsSkill("mofashufu"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("TrickCard");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ShufuCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        ShufuCard *card = new ShufuCard;
        card->addSubcard(card_item->getCard());
        return card;
    }
};

class Jiejin: public TriggerSkill{
public:
    Jiejin(): TriggerSkill("mofajiejin$"){
        events << CardAsked;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasLordSkill("mofajiejin");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        QString asked = data.toString();
        Room *room = player->getRoom();
        if(asked != "jink" &&
           asked != "slash" &&
           asked != "peach" &&
           asked != "analeptic")
            return false;


        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if(p->getKingdom() == "qingjiao")
                targets << p;
        }

        if(targets.length() == 0 || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        foreach(ServerPlayer *target, targets){
            QString choice = room->askForChoice(target, objectName(), "yes+no");
            if(choice == "no")
                return false;

            const Card *card = room->askForCard(target, ".", "@jiejin-give", false);
            if(card){
                room->moveCardTo(card, player, Player::Hand, false);
                return false;
            }
        }

        return false;
    }

};

class Liangzi: public TriggerSkill{
public:
    Liangzi():TriggerSkill("liangzijiuchan"){
        events << Damaged;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getRoom()->findPlayerBySkillName(objectName());
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getHp() >= player->getLostHp() || player->isDead())
            return false;

        Room *room = player->getRoom();
        ServerPlayer *leiyasita = room->findPlayerBySkillName(objectName());
        if(leiyasita->getHandcardNum() == 0 ||
           leiyasita == player ||
           !room->askForSkillInvoke(leiyasita, objectName(), data))
            return false;

        const Card *card = room->askForCard(leiyasita, "slash", "@jiuchan-slash");
        if(card){
            CardUseStruct use;
            use.from = leiyasita;
            use.to << player;
            use.card = card;
            room->useCard(use);
        }

        return false;
    }
};

class Xushu: public TriggerSkill{
public:
    Xushu():TriggerSkill("xushuxingshi$"){
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStar damage = data.value<DamageStar>();
        Room *room = player->getRoom();
        ServerPlayer *lord = room->getLord();

        if((damage->from && damage->from->getKingdom() == "school")
            || damage->to->getKingdom() == "school"){
            if(room->askForSkillInvoke(lord, "xushuxingshi")){
                ServerPlayer *target = room->askForPlayerChosen(lord, room->getOtherPlayers(lord), objectName());
                room->swapSeat(lord, target);
            }
        }

        return false;
    }
};

class Fengsuo: public TriggerSkill{
public:
    Fengsuo():TriggerSkill("zhuijifengsuo"){
        events << Predamaged;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Normal)
            return false;
        if(!room->askForSkillInvoke(player, objectName(), data))
            return false;

        QList<int> cards = room->getNCards(1);
        room->fillAG(cards);

        int ai_delay = Config.AIDelay;
        Config.AIDelay = 0;

        const Card *get = Sanguosha->getCard(cards.first());

        LogMessage log;
        log.type = "$Fengsuo";
        log.from = player;
        log.to << damage.from;
        log.card_str = get->getEffectIdString();
        room->sendLog(log);

        QString suit = get->getSuitString();
        QString pattern = QString(".%1").arg(suit.at(0).toUpper());
        QString prompt = "@fengsuo-hit:" + player->objectName();
        const Card *card = room->askForCard(damage.from, pattern, prompt, false);

        Config.AIDelay = ai_delay;
        room->broadcastInvoke("clearAG");
        room->throwCard(cards.first());
        if(!card){
            LogMessage log2;
            log2.type = "#FengsuoResult";
            log2.to << damage.from;
            room->sendLog(log2);

            return true;
        }
        else
            room->throwCard(card, true);

        return false;
    }
};

WuzhengCard::WuzhengCard(){
    once = true;
}

void WuzhengCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *target = effect.to, *source = effect.from;
    Room *room = target->getRoom();

    const Card *card = room->askForCard(source, ".", "@wuzheng", false);
    if(!card)
        return;

    room->moveCardTo(card, target, Player::Hand, false);

    const Card *slash = room->askForCard(target, "slash", "@wuzheng-answer");
    if(!slash){
        if(target->getCards("e").length() > 1){
            for(int i = 0; i < 2; i++){
                int card_id = room->askForCardChosen(target, target, "e", "wuzhengshijie");
                room->throwCard(card_id, true);
            }
        }
        else{
            DamageStruct damage;
            damage.from = source;
            damage.to = target;
            damage.damage = 1;
            room->damage(damage);
        }
    }

}

class Wuzheng: public ZeroCardViewAsSkill{
public:
    Wuzheng():ZeroCardViewAsSkill("wuzhengshijie"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("WuzhengCard") && player->getHandcardNum() > 0;
    }

    virtual const Card *viewAs() const{
        return new WuzhengCard;
    }
};

class Jushu: public TriggerSkill{
public:
    Jushu():TriggerSkill("shiershitujushushi"){
        events << Damaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        Room *room = player->getRoom();
        if(!damage.from ||
           room->getCurrent() != damage.from ||
           !room->askForSkillInvoke(player, objectName(), data))
            return false;

        QString choice = room->askForChoice(player, objectName(), "phasechange+discard");
        if(choice == "phasechange"){
            room->setTag("jushu", true);
        }
        else
            damage.from->throwAllEquips();

        return false;
    }
};

class Weiwang: public TriggerSkill{
public:
    Weiwang():TriggerSkill("zhongjiaodeweiwang$"){
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();


        if(event == HpRecover){
            RecoverStruct recover = data.value<RecoverStruct>();
            if(recover.who->getKingdom() != "agency")
                return false;

            LogMessage log;
            log.type = "#WeiwangEffect";
            log.from = player;
            room->sendLog(log);

            recover.who->drawCards(1);
        }
        if(event == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.from == NULL || damage.from->getKingdom() != "agency")
                return false;

            LogMessage log;
            log.type = "#WeiwangEffect";
            log.from = player;
            room->sendLog(log);

            int n = damage.from->getHandcardNum();
            room->askForDiscard(damage.from, objectName(), qMin(n, 1));
        }

        return false;
    }
};

class Tianfa: public TriggerSkill{
public:
    Tianfa():TriggerSkill("tianfashushi"){
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        Room *room = player->getRoom();
        if(!damage.from)
            return false;

        LogMessage log;
        log.type = "#TianfaEffect";
        log.from = player;
        log.to << damage.from;
        room->sendLog(log);

        if(!room->askForDiscard(damage.from, objectName(), 2, true, true))
            room->loseHp(damage.from);

        return false;
    }
};

class Dunqi: public TriggerSkill{
public:
    Dunqi():TriggerSkill("fengzhidunqi"){
        events << CardDiscarded;
        frequency = Compulsory;
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getPhase() != Player::Discard)
            return false;

        CardStar card = data.value<CardStar>();
        if(card->getSubcards().length() >= player->getHp() && player->isWounded()){
            LogMessage log;
            log.type = "#DunqiRecover";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);

            RecoverStruct recover;
            recover.who = player;
            recover.card = card;
            room->recover(player, recover);
        }

        return false;
    }
};

JiushuCard::JiushuCard(){
    once = true;
}

bool JiushuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return to_select->isWounded();
}

void JiushuCard::onEffect(const CardEffectStruct &effect) const{
    int hp = effect.to->getLostHp();
    if(hp == 0)
        return;

    Room *room = effect.from->getRoom();
    room->loseHp(effect.from, hp);
    RecoverStruct recover;
    recover.who = effect.from;
    recover.recover = hp;
    room->recover(effect.to, recover);

    effect.from->setMark("jiushu_acc", hp);
}

class Jiushu: public ZeroCardViewAsSkill{
public:
    Jiushu():ZeroCardViewAsSkill("cibeidejiushu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("JiushuCard");
    }

    virtual const Card *viewAs() const{
        return new JiushuCard();
    }
};

class JiushuAcc: public TriggerSkill{
public:
    JiushuAcc():TriggerSkill("#jiushu-acc"){
        events << Predamage << PhaseChange;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getMark("jiushu_acc") == 0)
            return false;

        if(event == Predamage){
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage += player->getMark("jiushu_acc");
            data = QVariant::fromValue(damage);

            LogMessage log;
            log.type = "#JiushuAcc";
            log.from = player;
            log.to << damage.to;
            log.arg = QString::number(player->getMark("jiushu_acc"));
            room->sendLog(log);

            player->removeMark("jiushu_acc");
        }
        if(event == PhaseChange && player->getPhase() == Player::Finish)
            player->removeMark("jiushu_acc");

        return false;
    }
};

class Shensheng: public TriggerSkill{
public:
    Shensheng():TriggerSkill("shenshengzhiyou"){
        events << Damaged << SlashEffect;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        LogMessage log;
        log.type = "#ShenshengEffect";

        if(event == Damaged){
            DamageStruct damage = data.value<DamageStruct>();

            if(!player->hasSkill(objectName()) || !damage.card->inherits("Slash"))
                return false;

            log.from = player;
            room->sendLog(log);

            damage.to = player->getNextAlive();
            room->damage(damage);
        }
        if(event == SlashEffect){
            ServerPlayer *huo = room->findPlayerBySkillName(objectName());
            if(huo == NULL || huo->getNextAlive() != player)
                return false;

            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(huo)){
                if(huo->inMyAttackRange(p))
                    targets << p;
            }
            if(targets.isEmpty())
                return false;

            log.from = huo;
            room->sendLog(log);

            ServerPlayer *target = room->askForPlayerChosen(huo, targets, objectName());
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.from = huo;
            effect.to = target;
            room->slashEffect(effect);
        }

        return false;
    }
};

ChuxingCard::ChuxingCard(){
    once = true;
}

bool ChuxingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QStringList players = Self->getFlags().split("+");
    if(targets.isEmpty())
        return (players.contains(to_select->objectName()) || Self->inMyAttackRange(to_select))
            && !Self->isProhibited(to_select, new Slash(Card::NoSuit, 0));
    else{
        bool selected_in_range = false;
        foreach(const Player *target, targets){
            if(!players.contains(target->objectName())){
                selected_in_range = true;
            }
        }

        if(selected_in_range)
            return players.contains(to_select->objectName()) && targets.length() <= players.length();
    }

    return (players.contains(to_select->objectName()) || Self->inMyAttackRange(to_select))
        && !Self->isProhibited(to_select, new Slash(Card::NoSuit, 0))
        && targets.length() <= players.length();
}

void ChuxingCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("guangzhichuxing");
    CardUseStruct use;
    use.from = source;
    use.to = targets;
    use.card = slash;
    room->useCard(use, false);
}

class ChuxingViewAsSkill: public ZeroCardViewAsSkill{
public:
    ChuxingViewAsSkill(): ZeroCardViewAsSkill("guangzhichuxing"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@chuxing";
    }

    virtual const Card *viewAs() const{
        ChuxingCard *card = new ChuxingCard;
        card->setSkillName("guangzhichuxing");
        return card;
    }
};

class Chuxing: public TriggerSkill{
public:
    Chuxing():TriggerSkill("guangzhichuxing"){
        events << DrawNCards << SlashEffected;
        view_as_skill = new ChuxingViewAsSkill;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == SlashEffected && player->getPhase() == Player::NotActive){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if(effect.from)
                room->setPlayerFlag(player, effect.from->objectName());
        }
        else if(event == DrawNCards){
            if(player->getFlags().isEmpty())
                return false;

            bool used = room->askForUseCard(player, "@chuxing", "@chuxing-card");

            QStringList players = player->getFlags().split("+");
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if(players.contains(p->objectName()))
                    room->setPlayerFlag(player, "-" + p->objectName());

            if(used)
                return true;
        }

        return false;
    }
};

DayanshuCard::DayanshuCard(){
    once = true;
}

bool DayanshuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng();
}

void DayanshuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    QList<const Card *> cards = effect.to->getHandcards();
    QList<int> card_ids;
    foreach(const Card *card, cards){
        card_ids << card->getEffectiveId();
    }

    room->fillAG(card_ids, effect.from);

    int card_id = room->askForAG(effect.from, card_ids, false, objectName());
    room->showCard(effect.to, card_id);

    room->setPlayerMark(effect.from, "dayanshu", card_id);
    room->setPlayerFlag(effect.from, "dayanshu");

    room->broadcastInvoke("clearAG");
}

class Dayanshu: public ViewAsSkill{
public:
    Dayanshu():ViewAsSkill("jinsedayanshu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if(player->hasUsed("DayanshuCard") && player->hasFlag("dayanshu")){
            int card_id = player->getMark("dayanshu");
            const Card *card = Sanguosha->getCard(card_id);
            return card->isAvailable(player);
        }else
            return true;
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(Self->hasUsed("DayanshuCard") && Self->hasFlag("dayanshu")){
            int plus = 0;
            foreach(CardItem *card_item, selected){
                plus += card_item->getFilteredCard()->getNumber();
            }

            int card_id = Self->getMark("dayanshu");
            const Card *card = Sanguosha->getCard(card_id);

            if(plus == card->getNumber())
                return false;

            return to_select->getFilteredCard()->getNumber() <= card->getNumber()-plus;
        }else
            return false;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(Self->hasUsed("DayanshuCard")){
            if(!Self->hasFlag("dayanshu"))
                return false;

            if(cards.length() == 0)
                return NULL;

            int card_id = Self->getMark("dayanshu");
            const Card *card = Sanguosha->getCard(card_id);
            const Card *first = cards.first()->getFilteredCard();

            Card *new_card = Sanguosha->cloneCard(card->objectName(), first->getSuit(), first->getNumber());
            new_card->addSubcards(cards);
            new_card->setSkillName(objectName());
            return new_card;
        }
        else
            return new DayanshuCard;
    }
};

class Lianjinshu: public TriggerSkill{
public:
    Lianjinshu():TriggerSkill("lianjinshu"){
        events << CardUsed << CardDiscarded << FinishJudge;
        frequency = Frequent;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }


    QList<const Card *> getCards(const Card *card) const{
        QList<const Card *> cards;

        if(!card->isVirtualCard()){
            cards << card;

            return cards;
        }

        foreach(int card_id, card->getSubcards()){
            const Card *c = Sanguosha->getCard(card_id);
                cards << c;
        }

        return cards;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QList<const Card *> cards;
        if(player->getPhase() == Player::Discard)
            return false;

        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            const SkillCard *skill_card = qobject_cast<const SkillCard *>(use.card);
            if(skill_card && skill_card->subcardsLength() > 0 && skill_card->willThrow()){
                cards = getCards(skill_card);
            }
        }else if(event == CardDiscarded){
            const Card *card = data.value<CardStar>();
            if(card->subcardsLength() == 0)
                return false;

            cards = getCards(card);
        }else if(event == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(room->getCardPlace(judge->card->getEffectiveId()) == Player::DiscardedPile
               && judge->card->getSuit() == Card::Club)
               cards << judge->card;
        }

        if(cards.isEmpty())
            return false;

        ServerPlayer *aure = room->findPlayerBySkillName(objectName());
        if(aure && aure->askForSkillInvoke(objectName(), data)){
            foreach(const Card *club, cards)
                aure->obtainCard(club);
        }

        return false;
    }
};

class Anyu: public TriggerSkill{
public:
    Anyu():TriggerSkill("anyujiedu"){
        events << StartJudge;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        ServerPlayer *aquinas = room->findPlayerBySkillName(objectName());
        if(aquinas == NULL || !room->askForSkillInvoke(aquinas, objectName()))
            return false;

        int n = 0;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if(p->getHp()>n)
                n = p->getHp();
        }

        QList<int> card_ids = room->getNCards(n);
        room->doGuanxing(aquinas, card_ids, true);

        return false;
    }
};

class Fazhishu: public TriggerSkill{
public:
    Fazhishu():TriggerSkill("fazhishuanyu"){
        events << Damaged;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature == DamageStruct::Normal)
            return false;

        ServerPlayer *aquinas = room->findPlayerBySkillName(objectName());
        if(aquinas == NULL || !room->askForSkillInvoke(aquinas, objectName()))
            return false;

        QString choice = room->askForChoice(aquinas, objectName(), "black+red");
        QList<int> cards = room->getNCards(1);
        const Card *card = Sanguosha->getCard(cards.first());

        LogMessage log;
        log.type = "$Fazhishu";
        log.from = aquinas;
        log.card_str = QString::number(cards.first());
        room->sendLog(log);

        if((choice == "black" && card->isBlack()) ||
            (choice == "red" && card->isRed()))
            aquinas->obtainCard(card);
        else
            room->doGuanxing(player, cards, true, true);

        return false;
    }
};

class Duoluo: public TriggerSkill{
public:
    Duoluo():TriggerSkill("tianshiduoluo"){
        events << DrawNCards;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        ServerPlayer *mixia = target->getRoom()->findPlayerBySkillName(objectName());
        if(mixia && mixia != target){
            if(target->distanceTo(mixia) <= 1)
                return true;
        }

        return false;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        ServerPlayer *mixia = room->findPlayerBySkillName(objectName());
        LogMessage log;
        log.type = "#DuoluoInvoke";
        log.from = mixia;
        room->sendLog(log);

        if(!room->askForDiscard(player, objectName(), 1, true, true))
            data = data.toInt() - 1;

        return false;
    }
};

class HeiShouhu: public OneCardViewAsSkill{
public:
    HeiShouhu():OneCardViewAsSkill("heizhishouhu"){

    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped() &&
            to_select->getFilteredCard()->isBlack();

    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        Card *new_card = NULL;

        switch(ClientInstance->getStatus()){
        case Client::Playing:{
                new_card = new Slash(card->getSuit(), card->getNumber());
                break;
            }
        case Client::Responsing:{
                QString pattern = ClientInstance->getPattern();
                if(pattern == "jink")
                    new_card = new Jink(card->getSuit(), card->getNumber());
                else if(pattern == "slash")
                    new_card = new Slash(card->getSuit(), card->getNumber());

                break;
            }
        default:
            break;
        }

        if(new_card){
            new_card->addSubcard(card);
            new_card->setSkillName(objectName());
            return new_card;
        }

        return NULL;
    }
};

SuqingCard::SuqingCard(){
    once = true;
    target_fixed = true;
}

void SuqingCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    QList<ServerPlayer *> others = room->getOtherPlayers(source);
    int card_id = subcards.first();
    const Card *fcard = Sanguosha->getCard(card_id);

    int number = fcard->getNumber();
    room->throwCard(this);
    source->loseAllMarks("@annihilate");


    foreach(ServerPlayer *p, others){
        room->setPlayerMark(p, "suqing", number);
 /*       const Card * card = room->askForCard(p, ".suqing", "@suqing-card", false);
        if(card){
            number = card->getNumber();
            room->throwCard(card->getEffectiveId(), true);
        }
        else
            room->loseHp(p);
*/
        QList<int> cards_id;
        foreach(const Card *card, p->getHandcards()){
            if(card->getNumber() >= number)
                cards_id << card->getEffectiveId();
        }

        int card_id = -1;
        if(cards_id.isEmpty())
            room->askForCard(p, "null", "@suqing-card", false);
        else
            card_id = room->doHandcardsChosen(p, cards_id, objectName());

        if(card_id == -1)
            room->loseHp(p);
        else{
            const Card *card = Sanguosha->getCard(card_id);
            number = card->getNumber();
            room->throwCard(card_id, true);
        }
        p->removeMark("suqing");
    }
}

class SkillPattern: public CardPattern{
public:
    virtual bool match(const Player *player, const Card *card) const{
        int number = player->getMark("suqing");
        return card->getNumber() >= number;
    }
};

class Suqing: public OneCardViewAsSkill{
public:
    Suqing():OneCardViewAsSkill("shenzhisuqing"){
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@annihilate") > 0 && player->getHandcardNum() == 1;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        SuqingCard *new_card = new SuqingCard;
        new_card->addSubcard(card);
        return new_card;
    }
};

class WangluoStart: public GameStartSkill{
public:
    WangluoStart():GameStartSkill("#wangluo-start"){
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual void onGameStart(ServerPlayer *player) const{
        player->setMark("net_hp", player->getHp());
        player->setMark("net_maxhp", player->getMaxHP());
        player->setXueyi(1);
    }
};

WangluoCard::WangluoCard(){
    once = true;
    target_fixed = true;
}

void WangluoCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    int hp = source->getHp(), new_hp = source->getMark("net_hp");
    int max_hp = source->getMaxHP(), new_max_hp = source->getMark("net_maxhp");
    source->setFlags("net");

    QList<int> old_cards = source->getPile("net");

    QList<const Card *> cards = source->getCards("ej");
    foreach(const Card *card, cards)
        source->addToPile("net", card->getEffectiveId(), false);

    foreach(int card_id, old_cards){
        const Card *card = Sanguosha->getCard(card_id);
        if(card->inherits("DelayedTrick"))
            room->moveCardTo(card, source, Player::Judging);
        else if(card->inherits("EquipCard"))
            room->moveCardTo(card, source, Player::Equip);
    }

    room->output(QString::number(new_hp));
    room->setPlayerProperty(source, "hp", new_hp);
    room->setPlayerProperty(source, "maxhp", new_max_hp);
    source->setMark("net_hp", hp);
    source->setMark("net_maxhp", max_hp);
    source->setFlags("-net");
}

class Wangluo: public ZeroCardViewAsSkill{
public:
    Wangluo():ZeroCardViewAsSkill("yubanwangluo"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return (player->usedTimes("WangluoCard") < 2);
    }

    virtual const Card *viewAs() const{
        return new WangluoCard;
    }
};

class Dianqi: public TriggerSkill{
public:
    Dianqi(): TriggerSkill("quexiandianqi"){
        events << Damaged;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getRoom()->findPlayerBySkillName(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        Room *room = player->getRoom();
        ServerPlayer *meimei = room->findPlayerBySkillName(objectName());
        if(meimei == NULL)
            return false;

        if(damage.nature == DamageStruct::Thunder && room->askForSkillInvoke(meimei, objectName())){
            for(int i = 0; i < damage.damage; i++){
                int n = meimei->getMark("net_maxhp")-meimei->getMark("net_hp");
                n += meimei->getLostHp();
                meimei->drawCards(qMax(n, 1));
            }
        }

        return false;
    }
};

class NaoliPattern: public CardPattern{
public:
    virtual bool match(const Player *player, const Card *card) const{
        QList<int> values = player->getTempData(player);

        return values.contains(card->getEffectiveId());
    }
};

class Chaonaoli: public TriggerSkill{
public:
    Chaonaoli(): TriggerSkill("chaonaoli"){
        events << DrawCardsDone;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(room->getTag("FirstRound").toBool())
            return false;
        if(!player->faceUp() || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        int n = data.toInt();
        QList<int> cards_id = player->handCards().mid(player->getHandcardNum() - n);

        int id = room->doHandcardsChosen(player, cards_id, objectName());
        if(id != -1){
            room->throwCard(id, true);

            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            target->gainMark("@brain");

            QString choice = room->askForChoice(player, objectName(), "judge+draw+play+discard");
            if(choice == "judge")
                target->setFlags("naoli_judge");
            else if(choice == "draw")
                target->setFlags("naoli_draw");
            else if(choice == "play")
                target->setFlags("naoli_play");
            else if(choice == "discard")
                target->setFlags("naoli_discard");

            LogMessage log;
            log.type = "#NaoliUsed";
            log.from = player;
            log.to << target;
            log.arg = choice;

            player->turnOver();
        }

        return false;
    }
};

class NaoliEffect: public TriggerSkill{
public:
    NaoliEffect(): TriggerSkill("#naoli-effect"){
        events << PhaseChange;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getMark("@brain") > 0;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Start)
            return false;

        if(player->hasFlag("naoli_judge"))
            player->skip(Player::Judge);
        else if(player->hasFlag("naoli_draw"))
            player->skip(Player::Draw);
        else if(player->hasFlag("naoli_play"))
            player->skip(Player::Play);
        else if(player->hasFlag("naoli_discard"))
            player->skip(Player::Discard);

        player->loseAllMarks("@brain");
        return false;
    }
};

class Shenquan: public TriggerSkill{
public:
    Shenquan(): TriggerSkill("muyuanshenquan"){
        events << TurnOverDone;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        if(!player->faceUp() || !room->askForSkillInvoke(player, objectName()))
            return false;

        CardUseStruct card_use;
        room->activate(player, card_use);
        if(card_use.isValid()){
            room->useCard(card_use);

            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if(p->getWeapon() != NULL)
                    targets << p;

            QString choice = room->askForChoice(player, objectName(), "weapon+drawone");

            if(choice == "drawone")
                player->drawCards(2);
            else{
                if(targets.isEmpty())
                    return false;

                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                const Weapon *weapon = target->getWeapon();
                room->throwCard(weapon->getEffectiveId(), true);
            }
        }

        return false;
    }
};

class Zhongduan: public TriggerSkill{
public:
    Zhongduan(): TriggerSkill("wangluozhongduan"){
        events << CardDiscarded << PhaseChange << DrawNCards << DrawCardsDone;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == CardDiscarded){
            if(player->getPhase() == Player::Discard)
                room->setPlayerMark(player, "zhongduan", 0);
        }
        else if(event == PhaseChange){
            if(player->getPhase() == Player::Finish){
                if(player->getMark("zhongduan") == 0 || !room->askForSkillInvoke(player, objectName(), data)){
                    room->setPlayerMark(player, "zhongduan", 0);
                    return false;
                }
            }
        }
        else if(event == DrawNCards){
            data = QVariant::fromValue(data.toInt()+player->getMark("zhongduan"));
        }
        else if(event == DrawCardsDone){
            if(player->getPhase() == Player::Draw){
                int n = player->getMark("zhongduan");
                room->setPlayerMark(player, "zhongduan", n+1);
            }
        }

        return false;
    }
};

GetiCard::GetiCard(){
    once = true;
}

bool GetiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return to_select->isWounded() && to_select != Self;
}

void GetiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    RecoverStruct recover;
    recover.card = this;
    recover.who = source;
    room->recover(targets.first(), recover);
}

class Geti: public ViewAsSkill{
public:
    Geti(): ViewAsSkill("shangweigeti"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("GetiCard");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        GetiCard *card = new GetiCard;
        card->addSubcards(cards);
        card->setSkillName(objectName());
        return card;
    }
};

BenghuaiCard::BenghuaiCard(){
    once = true;
}

bool BenghuaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(subcards.isEmpty() && Self->getMark("@gun")>0)
        return true;
    else if((subcards.length() == 2) && (Self->getMark("@gun") == 0))
        return to_select == Self;

    return false;
}

void BenghuaiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();

    if(subcards.isEmpty() && source->getMark("@gun")>0){
        target->drawCards(3);
        source->loseAllMarks("@gun");
    }
    else if(subcards.length() == 2){
        room->throwCard(this);
        source->gainMark("@gun", 1);
    }
}

class Benghuai: public ViewAsSkill{
public:
    Benghuai(): ViewAsSkill("shengrenbenghuai"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("BenghuaiCard");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return new BenghuaiCard;
        else if(cards.length() == 2){
            BenghuaiCard *card = new BenghuaiCard;
            card->addSubcards(cards);
            return card;
        }
        else
            return NULL;
    }
};

QizuiCard::QizuiCard(){
    once = true;
}

bool QizuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.isEmpty())
        return Self->inMyAttackRange(to_select);
    else
        return to_select != Self && targets.length() <= Self->getLostHp();
}

void QizuiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> left = targets;
    while(left.length() > 1){
        foreach(ServerPlayer *p, targets){
            if(!left.contains(p))
                continue;

            if(!room->askForCard(p, "slash", "@qizui-slash")){
                DamageStruct damage;
                damage.card = this;
                damage.from = source;
                damage.to = p;
                room->damage(damage);

                left.removeOne(p);
                if(left.length() == 1)
                    return;
            }
        }
    }
}

class Qizui: public ZeroCardViewAsSkill{
public:
    Qizui():ZeroCardViewAsSkill("qijiaoqizui"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("QizuiCard");
    }

    virtual const Card *viewAs() const{
        QizuiCard *card = new QizuiCard;
        card->setSkillName(objectName());
        return card;
    }
};

class Nixi: public TriggerSkill{
public:
    Nixi(): TriggerSkill("nixishouhu"){
        events << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        player->setFlags("nixi_start");
        if(player->getPhase() != Player::Finish || !room->askForSkillInvoke(player, objectName(), data))
            return false;

        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAlivePlayers())
            if(p->isWounded())
                targets << p;

        QList<int> card_ids = room->getNCards(qMax(targets.length(), 1));
        foreach(int card_id, card_ids){
            player->obtainCard(Sanguosha->getCard(card_id));
            player->addToPile("nixi", card_id, false);
        }

        return false;
    }
};

class NixiEffect: public TriggerSkill{
public:
    NixiEffect():TriggerSkill("#nixi-effect"){
        events << CardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getRoom()->findPlayerBySkillName("nixishouhu");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->getTypeId() != Card::Basic)
            return false;

        ServerPlayer *binmian = room->findPlayerBySkillName("nixishouhu");
        QList<int> cards_id = binmian->getPile("nixi");
        bool suit = false, number = false;
        bool suit_refused = false, number_refused = false;
        foreach(int card_id, cards_id){
            if(Sanguosha->getCard(card_id)->getSuit() == use.card->getSuit() && !suit_refused){
                room->fillAG(cards_id, binmian);
                QVariant s = QVariant::fromValue(use.card->getSuitString());
                room->setTag("nixi_suit", s);
                if(room->askForSkillInvoke(binmian, "nixishouhu", data)){
                    suit = true;
                    break;
                }
                else
                    suit_refused = true;
                room->setTag("nixi_suit", NULL);
                room->broadcastInvoke("clearAG");
            }
            else if(Sanguosha->getCard(card_id)->getNumber() == use.card->getNumber() && !number_refused){
                room->fillAG(cards_id, binmian);
                QVariant n = QVariant::fromValue(use.card->getNumber());
                room->setTag("nixi_number", n);
                if(room->askForSkillInvoke(binmian, "nixishouhu", data)){
                    number = true;
                    break;
                }
                else
                    number_refused = true;
                room->setTag("nixi_number", 0);
                room->broadcastInvoke("clearAG");
            }

            if(suit_refused && number_refused)
                return false;
        }

        if(!suit && !number)
            return false;

        room->broadcastInvoke("clearAG");

        foreach(int card_id, cards_id){
            if(suit){
                if(Sanguosha->getCard(card_id)->getSuit() == use.card->getSuit())
                    binmian->obtainCard(Sanguosha->getCard(card_id), false);
                else
                    room->throwCard(card_id, true);
            }
            else if(number){
                binmian->obtainCard(Sanguosha->getCard(card_id), false);
            }
        }

        if(suit){
            LogMessage log;
            log.type = "$NixiSuit";
            log.from = binmian;
            log.to << player;
            log.card_str = use.card->getEffectIdString();
            room->sendLog(log);
        }
        else if(number){
            LogMessage log;
            log.type = "$NixiNumber";
            log.from = binmian;
            log.to << player;
            log.card_str = use.card->getEffectIdString();
            room->sendLog(log);
        }

        return false;
    }
};

class Qishi: public DistanceSkill{
public:
    Qishi():DistanceSkill("huangjiaqishi")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(from->hasSkill(objectName()) && from->getPhase() == Player::Play)
            return -1;
        else if(to->hasSkill(objectName()) && to->getPhase() == Player::NotActive)
            return 1;
        else
            return 0;
    }
};

class Sishenzhe: public TriggerSkill{
public:
    Sishenzhe(): TriggerSkill("sishenzhe"){
        events << DrawNCards << Damaged;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == DrawNCards){
            if(player->getMark("sishen") == 0 || !room->askForSkillInvoke(player, objectName()))
                return false;
            int n = data.toInt();
            n += player->getMark("sishen")*2;
            room->setPlayerMark(player, "sishen", 0);
            data = QVariant::fromValue(n);
        }
        else{
            if(player->getPhase() != Player::NotActive || !room->askForSkillInvoke(player, objectName()))
                return false;

            DamageStruct damage = data.value<DamageStruct>();
            for(int i = 0; i < damage.damage; i++)
                player->addMark("sishen");
        }

        return false;
    }
};

IndexPackage::IndexPackage()
    :Package("Index")
{
    addCards();

    General *index = new General(this, "index$", "qingjiao", 3, false);
    index->addSkill(new Jiyi);
    index->addSkill(new Jiaohui);
    index->addSkill(new Yongchang);

    General *magnus = new General(this, "magnus", "qingjiao", 4);
    magnus->addSkill(new Huoyan);
    magnus->addSkill(new Ranshao);

    General *shenlie = new General(this, "shenlie", "qingjiao", 4, false);
    shenlie->addSkill(new Weishan);
    shenlie->addSkill(new WeishanClear);

    General *yuanchun = new General(this, "yuanchun", "qingjiao", 4);
    yuanchun->addSkill(new Kaifa);
    yuanchun->addSkill(new Beici);
    yuanchun->addSkill(new Zaisheng);

    General *wuhe = new General(this, "wuhe", "qingjiao", 3, false);
    wuhe->addSkill(new Benghuai);
    wuhe->addSkill(new MarkAssignSkill("@gun", 1));
    wuhe->addSkill(new Qizui);

    General *cromwell = new General(this, "cromwell", "qingjiao", 4, false);
    cromwell->addSkill(new Shixiang);

    General *agnese = new General(this, "agnese", "qingjiao", 4, false);
    agnese->addSkill(new Five);

    General *aquinas = new General(this, "aquinas", "qingjiao", 3, false);
    aquinas->addSkill(new Anyu);
    aquinas->addSkill(new Fazhishu);

    General *zhaizi = new General(this, "zhaizi", "qingjiao");
    zhaizi->addSkill(new Shenyin);
    zhaizi->addSkill(new Shushi);

    General *luola = new General(this, "luola$", "qingjiao", 4, false);
    luola->addSkill(new Shufu);
    luola->addSkill(new Jiejin);

    General *lisha = new General(this, "lisha", "qingjiao", 3, false);
    lisha->addSkill(new Qishi);
    lisha->addSkill(new Sishenzhe);

    /////////////////////////////////////////////////////////////////////////////////////////

    General *dangma = new General(this, "dangma", "school");
    dangma->addSkill(new ImagineKiller);
    dangma->addSkill(new ZeroLevel);

    General *qiusha = new General(this, "qiusha", "school", 3, false);
    qiusha->addSkill(new Xixue);
    qiusha->addSkill(new Cunzai);

    General *meiqin = new General(this, "meiqin", "school", 3, false);
    meiqin->addSkill(new DianciPao);
    meiqin->addSkill(new Pingzhang);

    General *shifeng = new General(this, "shifeng", "school", 4 ,false);
    shifeng->addSkill(new Xinling);

    General *heizi = new General(this, "heizi", "school", 3 ,false);
    heizi->addSkill(new Yidong);

    General *chuchun = new General(this, "chuchun", "school", 3, false);
    chuchun->addSkill(new Baocun);
    chuchun->addSkill(new Shouhu);
    chuchun->addSkill(new ShouhuSkill);

    General *xiaomeng = new General(this, "xiaomeng", "school", 3, false);
    xiaomeng->addSkill(new Shendian);
    xiaomeng->addSkill(new ShendianEffect);
    xiaomeng->addSkill(new Jitiyizhi);

    General *guanggui = new General(this, "guanggui", "school");
    guanggui->addSkill(new Shuimian);

    General *zuotian = new General(this, "zuotian", "school", 4, false);
    zuotian->addSkill(new Skill("lingkonglishi", Skill::Compulsory, "zuotian"));
    zuotian->addSkill(new MarkAssignSkill("@upper", 1));
    zuotian->addSkill(new Levelupper);
    zuotian->addSkill(new Upper);

    General *guangzi = new General(this, "guangzi", "school", 3, false);
    guangzi->addSkill(new StrongKonglishi);
    guangzi->addSkill(new Baopo);

    General *junba = new General(this, "junba", "school");
    junba->addSkill(new Skill("niandongpaodan", Skill::Compulsory, "junba"));
    junba->addSkill(new Hunluan);

    General *yubansister = new General(this, "yubansister", "school", 3, false);
    yubansister->addSkill(new Cilijuji);
    yubansister->addSkill(new CiliSlash);
    yubansister->addSkill(new Xuji);

    General *meimei = new General(this, "meimei", "school", 3, false);
    meimei->addSkill(new Wangluo);
    meimei->addSkill(new WangluoStart);
    meimei->addSkill(new Dianqi);

    General *last = new General(this, "last", "school", 3, false);
    last->addSkill(new Zhongduan);
    last->addSkill(new Geti);

    General *meiwei = new General(this, "meiwei", "school", 4, false);
    meiwei->addSkill(new Toushi);

    General *yaleisita = new General(this, "yaleisita$", "school");
    yaleisita->addSkill(new Liangzi);
    yaleisita->addSkill(new Xushu);
/////////////////////////////////////////////////////////////////////////////////////

    General *yifang = new General(this, "yifang$", "darkness");
    yifang->addSkill(new Shiliang);
    yifang->addSkill(new Tongxing);

    General *danxi = new General(this, "danxi", "darkness", 3, false);
    danxi->addSkill(new Zuobiao);
    danxi->addSkill(new Yizhi);

    General *aizhali = new General(this, "aizhali", "darkness");
    aizhali->addSkill(new Jinxing);
    aizhali->addSkill(new Gun);

    General *didu = new General(this, "didu", "darkness", 4);
    didu->addSkill(new Baiyi);
    didu->addSkill(new BaiyiStart);
    didu->addSkill(new BaiyiUse);
    didu->addSkill(new Wuzhi);

    General *chenli = new General(this, "chenli", "darkness", 3, false);
    chenli->addSkill(new Yuanzi);
    chenli->addSkill(new Lizipao);

    General *zuiai = new General(this, "zuiai", "darkness", 3, false);
    zuiai->addSkill(new Danqi);
    zuiai->addSkill(new DanqiCollect);

    General *hainiao = new General(this, "hainiao", "darkness", 4, false);
    hainiao->addSkill(new Baoqiang);
    hainiao->addSkill(new Diejia);

    General *lihou = new General(this, "lihou", "darkness", 5, false);
    lihou->addSkill(new Zhuiji);
    lihou->addSkill(new ZhuijiOn);
    lihou->addSkill(new Baozou);

    General *weizhi = new General(this, "weizhi", "darkness", 4, false);
    weizhi->addSkill(new Dinggui);
    weizhi->addSkill(new DingguiClear);

    General *muyuan = new General(this, "muyuan", "darkness");
    muyuan->addSkill(new Chaonaoli);
    muyuan->addSkill(new NaoliEffect);
    muyuan->addSkill(new Shenquan);

    General *binmian = new General(this, "binmian", "darkness");
    binmian->addSkill(new Nixi);
    binmian->addSkill(new NixiEffect);

 /////////////////////////////////////////////////////////////////////////////////////

    General *chunsheng = new General(this, "chunsheng", "agency", 4, false);
    chunsheng->addSkill(new Jiejing);
    chunsheng->addSkill(new JiejingTrigger);
    chunsheng->addSkill(new MarkAssignSkill("@lens", 1));

    General *binghua = new General(this, "binghua", "agency", 4, false);
    binghua->addSkill(new Tianshi);
    binghua->addSkill(new Buming);
    binghua->addSkill(new BumingSelect);

    General *oriana = new General(this, "oriana", "agency", 3, false);
    oriana->addSkill(new Fengsuo);
    oriana->addSkill(new Wuzheng);

    General *aolai = new General(this, "aolai", "agency", 3);
    aolai->addSkill(new Wangzuo);
    aolai->addSkill(new Unknown);

    General *aure = new General(this, "aure", "agency", 3);
    aure->addSkill(new Dayanshu);
    aure->addSkill(new Lianjinshu);

    General *mixia = new General(this, "mixia", "agency", 3, false);
    mixia->addSkill(new Duoluo);
    mixia->addSkill(new HeiShouhu);
    mixia->addSkill(new MarkAssignSkill("@annihilate", 1));
    mixia->addSkill(new Suqing);

    General *mading = new General(this, "mading$", "agency");
    mading->addSkill(new Jushu);
    mading->addSkill(new Weiwang);

    General *feng = new General(this, "feng", "agency", 3, false);
    feng->addSkill(new Tianfa);
    feng->addSkill(new Dunqi);

    General *shui = new General(this, "shui", "agency");
    shui->addSkill(new Jiushu);
    shui->addSkill(new JiushuAcc);

    General *huo = new General(this, "huo", "agency");
    huo->addSkill(new Shensheng);

    General *di = new General(this, "di", "agency");
    di->addSkill(new Chuxing);

    patterns[".suqing"] = new SkillPattern;
    patterns[".naoli"] = new NaoliPattern;

    addMetaObject<WeishanCard>();
    addMetaObject<BeiciCard>();
    addMetaObject<BaiyiCard>();
    addMetaObject<ShenyinCard>();
    addMetaObject<JinxingCard>();
    addMetaObject<ShendianCard>();
    addMetaObject<BaoqiangCard>();
    addMetaObject<ZhuijiCard>();
    addMetaObject<DingguiCard>();
    addMetaObject<ShufuCard>();
    addMetaObject<WuzhengCard>();
    addMetaObject<JiushuCard>();
    addMetaObject<DayanshuCard>();
    addMetaObject<SuqingCard>();
    addMetaObject<WangluoCard>();
    addMetaObject<GetiCard>();
    addMetaObject<HunluanCard>();
    addMetaObject<ChuxingCard>();
    addMetaObject<BenghuaiCard>();
    addMetaObject<QizuiCard>();
}

ADD_PACKAGE(Index)
