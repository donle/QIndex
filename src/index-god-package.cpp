#include "index-god-package.h"
#include "engine.h"
#include "client.h"
#include "carditem.h"
#include "serverplayer.h"
#include "clientplayer.h"
#include "settings.h"
#include "general.h"
#include "package.h"

class Momie: public TriggerSkill{
public:
    Momie(): TriggerSkill("momiezhisheng"){
        events << PhaseChange << DrawNCards;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == PhaseChange){
            if(player->getPhase() != Player::Start
               || !room->askForSkillInvoke(player, objectName())
               || !room->askForDiscard(player, objectName(), 1, true, true))
                return false;

            JudgeStruct judge;
            judge.reason = objectName();
            judge.good = true;
            judge.who = player;
            room->judge(judge);

            switch(judge.card->getSuit()){
            case Card::Heart:{
                    player->setFlags("momie_draw");
                    break;
                }
            case Card::Diamond:{
                    foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                        if(!p->isKongcheng())
                            room->askForDiscard(p, objectName(), qMin(2, p->getHandcardNum()));
                    }
                    break;
                }
            case Card::Spade:{
                    foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                        if(!p->getCards("e").isEmpty())
                            p->throwAllEquips();
                    }
                    break;
                }
            case Card::Club:{
                    foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                        p->drawCards(2);
                        p->turnOver();
                    }
                    break;
                }
            default:
                break;
            }
        }
        else{
            if(player->hasFlag("momie_draw")){
                player->setFlags("-momie_draw");
                int n = data.toInt()+3;
                data = QVariant::fromValue(n);
            }
        }

        return false;
    }
};

class Shuji: public TriggerSkill{
public:
    Shuji():TriggerSkill("zidongshuji"){
        events << PhaseChange;
        frequency = Frequent;
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(player->getPhase() == Player::Finish){
            if(player->getMark("shuji_played") > 0){
                player->removeMark("shuji_played");
                return false;
            }
            if(!player->isWounded() || !room->askForSkillInvoke(player, objectName()))
                return false;

            JudgeStruct judge;
            judge.pattern = QRegExp("(FreezeSlash|FireSlash|ThunderSlash|Slash|Jink|Peach|Analeptic|Shit):(.*):(.*)");
            judge.reason = objectName();
            judge.good = true;
            judge.who = player;
            room->judge(judge);

            if(judge.isGood()){
                player->addMark("shuji_play");

                LogMessage log;
                log.type = "#ShujiGood";
                log.from = player;
                log.arg  = objectName();
                room->sendLog(log);
            }
        }
        else if(player->getPhase() == Player::NotActive){
            if(player->getMark("shuji_play") > 0){
                player->removeMark("shuji_play");
                player->setMark("shuji_played", 1);

                LogMessage log;
                log.type = "#ShujiEffect";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                if(!player->faceUp())
                    player->turnOver();
                else
                    player->play();
            }
        }

        return false;
    }
};

class Qianzhao: public TriggerSkill{
public:
    Qianzhao(): TriggerSkill("qianzhaoganzhi"){
        events << CardEffected;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if(effect.multiple ||
           effect.from == effect.to ||
           !room->askForSkillInvoke(player, objectName(), data))
            return false;

        QList<int> card_id = room->getNCards(1);
        room->fillAG(card_id, player);
        int id = room->askForAG(player, card_id, true, objectName());
        room->broadcastInvoke("clearAG");
        if(id == -1){
            room->doGuanxing(player, card_id, true, true);
            if(player->isKongcheng() || !room->askForDiscard(player, objectName(), 1, true))
                return false;

            room->moveCardTo(effect.card, NULL, Player::DrawPile);
            return true;
        }
        else{
            const Card *card = room->askForCard(player, ".", "@qianzhao-card", false);
            if(card == NULL)
                return false;

            player->obtainCard(Sanguosha->getCard(id));
            room->moveCardTo(card, NULL, Player::DrawPile);
        }
        return false;
    }
};

class Mianyi: public ProhibitSkill{
public:
    Mianyi(): ProhibitSkill("qianglimianyi"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        return (card->inherits("Slash") && to->isKongcheng() && !from->getEquips().isEmpty()) ||
                (card->getTypeId() == Card::Trick && to->isKongcheng());
    }
};

class Buxing: public TriggerSkill{
public:
    Buxing(): TriggerSkill("buxingtizhi"){
        events << PhaseChange;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &) const{
        if(player->getPhase() != Player::Finish || !player->isWounded())
            return false;

        Room *room = player->getRoom();
        LogMessage log;
        log.type = "#BuxingTrigger";
        log.arg = objectName();
        log.from = player;
        room->sendLog(log);

        JudgeStruct judge;
        judge.who = player;
        judge.pattern = QRegExp("(.*):(heart|club|diamond):(.*)");
        judge.reason = objectName();
        judge.good = true;
        for(int i = 0; i < player->getLostHp(); i++){
            room->judge(judge);

            if(judge.isBad())
                player->throwAllHandCards();
            else{
                if(player->hasFlag("buxing_used"))
                    continue;

                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *p, room->getOtherPlayers(player))
                    if(player->distanceTo(p) <= 1)
                        targets << p;

                if(!targets.isEmpty()){
                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                    room->loseHp(target);
                }
                player->setFlags("buxing_used");
            }
        }

        return false;
    }
};

class Luolei: public TriggerSkill{
public:
    Luolei(): TriggerSkill("luolei"){
        events << CardLost;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        CardMoveStar move = data.value<CardMoveStar>();
        const Card *card = Sanguosha->getCard(move->card_id);
        if(player->getPhase() == Player::Discard ||
           card->getTypeId() != Card::Basic ||
           card->inherits("Slash") ||
           !player->isWounded() ||
           !room->askForSkillInvoke(player, objectName(), data))
            return false;

        for(int i = 0; i <= card->subcardsLength(); i++){
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
            JudgeStruct judge;
            judge.who = target;
            judge.pattern = QRegExp("(.*):(spade|club):(.*)");
            judge.reason = objectName();
            judge.good = true;
            for(int i = 0; i < player->getLostHp(); i++){
                room->judge(judge);

                if(judge.isGood()){
                    DamageStruct damage;
                    damage.from = player;
                    damage.to = target;
                    damage.nature = DamageStruct::Thunder;
                    damage.damage = 1;
                    if(judge.card->getSuit() == Card::Spade){
                        damage.damage++;

                        if(judge.card->getNumber() >= 2 && judge.card->getNumber() <= 9)
                            damage.damage++;
                    }

                    room->damage(damage);
                }
                if(target->isDead())
                    return false;
            }
        }
        return false;
    }
};

ShuiyiCard::ShuiyiCard(){
    mute = true;
}

bool ShuiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < 2;
}

void ShuiyiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    foreach(ServerPlayer *target, targets){
        room->setPlayerFlag(target, "shuiyi_target");
        foreach(const Card *equip, target->getCards("e"))
            room->moveCardTo(equip, target, Player::Hand);
        room->setPlayerFlag(target, "freezed");
    }
}

class ShuiyiView: public ZeroCardViewAsSkill{
public:
    ShuiyiView(): ZeroCardViewAsSkill("shuizhiyi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.endsWith("@@shuizhiyi");
    }

    virtual const Card *viewAs() const{
        return new ShuiyiCard;
    }
};

class Shuizhiyi: public TriggerSkill{
public:
    Shuizhiyi(): TriggerSkill("shuizhiyi"){
        events << PhaseChange;
        view_as_skill = new ShuiyiView;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::Start)
            return false;

        if(player->getRoom()->askForUseCard(player, "@@shuizhiyi", "@shuiyi-use"))
            player->skip(Player::Play);

        return false;
    }
};

class ShuiyiDamage: public TriggerSkill{
public:
    ShuiyiDamage(): TriggerSkill("#shuizhiyi"){
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasFlag("shuiyi_target") && target->hasFlag("freezed");
    }


    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        LogMessage log;
        log.type = "#ShuiyiTrigger";
        log.arg = "shuizhiyi";
        log.from = room->findPlayerBySkillName("shuizhiyi", true);
        room->sendLog(log);

        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if(!p->hasFlag("shuiyi_target"))
                continue;

            room->setPlayerFlag(p, "-shuiyi_target");
            DamageStruct damage = data.value<DamageStruct>();
            damage.to = p;
            room->damage(damage);
        }

        return false;
    }
};

class Abrahadabra: public TriggerSkill{
public:
    Abrahadabra(): TriggerSkill("abrahadabra"){
        events << PhaseChange << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *aiwass, QVariant &data) const{
        Room *room = aiwass->getRoom();

        LogMessage log;
        log.type = "#AbrahadabraEffect";
        log.from = aiwass;
        log.arg = objectName();

        if(event == PhaseChange){
            if(aiwass->getPhase() == Player::Finish){
                room->sendLog(log);

                ServerPlayer *target = room->askForPlayerChosen(aiwass, room->getOtherPlayers(aiwass), objectName());
                int has_fan = 0;
                if(target->getWeapon() && target->getWeapon()->objectName() == "folding_fan")
                    has_fan = 1;
                else if(aiwass->getWeapon() && aiwass->getWeapon()->objectName() == "folding_fan")
                    has_fan = 2;

                int hp = has_fan == 1 ? target->getHp()-1 : target->getHp();
                int maxhp = has_fan == 1 ? target->getMaxHP()-1 : target->getMaxHP();
                int hp2 = has_fan == 2 ? aiwass->getHp()-1 : aiwass->getHp();
                int maxhp2 = has_fan == 2 ? aiwass->getMaxHP()-1 : aiwass->getMaxHP();

                room->setPlayerProperty(target, "maxhp", has_fan == 1 ? ++maxhp2 : maxhp2);
                room->setPlayerProperty(target, "hp", has_fan == 1 ? ++hp2 : hp2);
                room->setPlayerProperty(aiwass, "maxhp", has_fan == 2 ? ++maxhp : maxhp);
                room->setPlayerProperty(aiwass, "hp", has_fan == 2 ? ++hp : hp);
            }
        }
        else{
            room->sendLog(log);

            QList<ServerPlayer *> targets;
            int max_hp = 0;
            foreach(ServerPlayer *player, room->getAlivePlayers()){
                if(player->getHp() > max_hp){
                    max_hp = player->getHp();
                    targets << player;
                }
                else if(player->getHp() == max_hp){
                    if(!targets.isEmpty() && targets.first()->getHp() < player->getHp())
                            targets.clear();

                    targets << player;
                }
            }

            ServerPlayer *target = room->askForPlayerChosen(aiwass, targets, objectName());
            room->loseHp(target, 4);
        }

        return false;
    }
};

IndexGodPackage::IndexGodPackage()
    :Package("IndexGod")
{
    General *godindex = new General(this, "godindex", "god", 3, false);
    godindex->addSkill(new Momie);
    godindex->addSkill(new Shuji);

    General *goddangma = new General(this, "goddangma", "god");
    goddangma->addSkill(new Qianzhao);
    goddangma->addSkill(new Mianyi);
    goddangma->addSkill(new Buxing);

    General *godmeiqin = new General(this, "godmeiqin", "god", 3, false);
    godmeiqin->addSkill(new Luolei);
    godmeiqin->addSkill(new Shuizhiyi);
    godmeiqin->addSkill(new ShuiyiDamage);

    General *aiwass = new General(this, "aiwass", "god", 8, false);
    aiwass->addSkill(new Abrahadabra);

    addMetaObject<ShuiyiCard>();
}

ADD_PACKAGE(IndexGod)
