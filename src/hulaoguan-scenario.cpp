#include "hulaoguan-scenario.h"
#include "engine.h"
#include "standard-skillcards.h"
#include "clientplayer.h"
#include "client.h"
#include "carditem.h"

ChongzhuCard::ChongzhuCard(){
    target_fixed = true;
}

void ChongzhuCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(1);
}

void ChongzhuCard::use(Room *room, ServerPlayer *player, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    CardEffectStruct effect;
    effect.from = effect.to = player;
    effect.card = this;

    room->cardEffect(effect);
}

class Chongzhu: public OneCardViewAsSkill{
public:
    Chongzhu():OneCardViewAsSkill("chongzhu"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("EquipCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        ChongzhuCard *card = new ChongzhuCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class ChongzhengSkip: public PhaseChangeSkill{
public:
    ChongzhengSkip():PhaseChangeSkill("#chongzheng-skip"){
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();

        if(player->getPhase() == Player::Start){
            if(player->getMark("@prepare") > 0 && player->getMark("@prepare") < 6){
                player->gainMark("@prepare");
                player->skip(Player::Draw);
                player->skip(Player::Play);
                player->skip(Player::Finish);

                return true;
            }

            if(player->getMark("@prepare") >= 6){
                player->loseAllMarks("@prepare");
                int draw = 6-player->getMaxHP();

                room->setPlayerProperty(player, "hp", player->getMaxHP());

                if(draw > 0)
                    player->drawCards(draw);

                if(!player->faceUp())   player->turnOver();
                if(player->isChained()) player->setChained(false);

                room->detachSkillFromPlayer(player, "#chongzheng-skip");
            }
        }

        return false;
    }
};

class Xiuluo: public PhaseChangeSkill{
public:
    Xiuluo():PhaseChangeSkill("xiuluo"){
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        QList<const Card *> judges = target->getCards("j");
        return target->hasSkill("xiuluo") && judges.length() > 0;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();

        if(target->getPhase() == Player::Start && room->askForSkillInvoke(target, objectName())){
            QList<const Card *> judges = target->getCards("j");
            QList<int> judges_id;
            foreach(const Card *judge, judges)
                judges_id << judge->getEffectiveId();

            room->fillAG(judges_id, target);

            while(!judges_id.isEmpty()){
                int card_id = room->askForAG(target, judges_id, true, "xiuluo");
                if(card_id == -1)
                    break;

                const Card *judge_card = Sanguosha->getCard(card_id);

                const QString judge_str = judge_card->getSuitString();
                QString pattern = QString(".%1").arg(judge_str.at(0).toUpper());
                const Card *throw_card = room->askForCard(target, pattern, "@xiuluo");

                if(throw_card){
                    room->moveCardTo(judge_card, NULL, Player::DiscardedPile);
                    judges_id.removeOne(card_id);
                }
            }

            target->invoke("clearAG");
        }
        return false;
    }
};

class Shenwei: public PhaseChangeSkill{
public:
    Shenwei():PhaseChangeSkill("shenwei"){
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::Draw){
            target->drawCards(2);
        }

        if(target->getPhase() == Player::Discard){
            target->setXueyi(2);
        }

        return false;
    }
};

class Shenji: public TriggerSkill{
public:
    Shenji():TriggerSkill("shenji"){
        events << CardLost;
    }


    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        if(player->getWeapon() == NULL)
            player->getRoom()->setPlayerFlag(player, "shenji");
        else
            player->getRoom()->setPlayerFlag(player, "-shenji");
        return false;
    }
};

class HulaoguanRule: public ScenarioRule{
public:
    HulaoguanRule(Scenario *scenario)
        :ScenarioRule(scenario)
    {

        events << GameStart << GameOverJudge << Death << PhaseChange
                << DrawNCards << CardGot << AskForPeachesDone << CardEffected
               << Predamaged  << Damaged << HpLost;
    }

    void removeAddroundMark(ServerPlayer *player) const{
        QList<ServerPlayer *> players = player->getRoom()->getAllPlayers();
        foreach(ServerPlayer *p, players){
            if(p->getMark("add") > 0)
                p->removeMark("add");
        }

    }

    void setChongzhuSkill(ServerPlayer *player) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach(ServerPlayer *p, players)
            room->acquireSkill(p, "chongzhu");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        switch(event){
        case GameStart:{
                if(player->isLord()){
                    room->transfigure(player, "godlvbu", true);
                    setChongzhuSkill(player);
                }
                break;
            }

        case GameOverJudge:{
                return true;
                break;
            }

        case PhaseChange:{
                if(room->getLord()->getMark("@wrath") > 0)  break;

                if(!player->isLord()){
                    if(player->getPhase() == Player::Start){
                        if(player->getMark("add") > 0)
                            player->removeMark("add");

                        if(!player->getNextAlive()->isLord())
                            player->getNextAlive()->addMark("add");
                    }

                    if(player->getPhase() == Player::NotActive){
                        if(room->getLord()->faceUp()){
                            room->setCurrent(room->getLord());
                            room->getLord()->play();
                        }
                        else
                            room->getLord()->turnOver();
                    }
                }

                else{
                    if(player->getPhase() == Player::Finish){
                        QList<ServerPlayer *> others = room->getOtherPlayers(player);
                        foreach(ServerPlayer *other, others){
                            if(other->getMark("add") > 0){
                                if(other->faceUp()){
                                    room->setCurrent(other);
                                    other->play();
                                }
                                else{
                                    other->turnOver();
                                    other->removeMark("add");

                                    if(!other->getNextAlive()->isLord())
                                        other->getNextAlive()->addMark("add");

                                    if(player->faceUp()){
                                        room->setCurrent(player);
                                        player->play();
                                    }
                                    else
                                        player->turnOver();
                                }

                                break;
                            }
                        }
                    }
                }
                break;
            }

        case DrawNCards:{
                if(room->getDrawpileNumber() <= 0){
                    room->output(QString::number(room->getDrawpileNumber()));
                    room->getLord()->addMark("shuffle");
                    if(room->getLord()->getMark("shuffle") >= 2)
                        room->gameOver(".");
               }

                break;
            }

        case AskForPeachesDone:{
                if(!player->isLord() && player->getHp() <= 0){
                    player->throwAllCards();
                    if(player->getMark("@prepare") <= 0){
                        player->gainMark("@prepare");
                        room->setPlayerProperty(player, "hp", 0);
                        room->acquireSkill(player, "#chongzheng-skip");
                    }

                    QList<ServerPlayer *> players = room->getAlivePlayers();
                    bool hasRebel = false;
                    foreach(ServerPlayer *each, players){
                        if(each->getRole() == "rebel" && each->getMark("@prepare") <= 0)
                            hasRebel = true;
                    }
                    if(!hasRebel)
                        room->gameOver("lord");

                    if(player->getMark("add") > 0){
                        player->removeMark("add");
                        if(!player->getNextAlive()->isLord())
                            player->getNextAlive()->addMark("add");
                    }
                    return true;
                }

                break;
            }

        case Death:{
                if(player->isLord())
                    room->gameOver("rebel");

                break;
        }

        case CardEffected:{
                if(player->getMark("@prepare") > 0)
                    return true;

                break;

            }

        case CardGot:{
                if(player->getMark("@prepare") > 0)
                    player->throwAllCards();

                break;
            }

        case Predamaged:{
                if(player->getMark("@prepare") > 0)
                    return true;

                break;
            }

        case HpLost:
        case Damaged:{
            if(player->isLord() && player->getMark("@wrath") <= 0){
                if(player->getHp() <= 4){
                    LogMessage log;
                    log.type = "#Baonu";
                    log.from = player;
                    room->sendLog(log);

                    room->setPlayerProperty(player, "maxhp", 4);

                    if(player->isChained())
                        player->setChained(false);

                    room->transfigure(player, "godlvbu2", true);
                    removeAddroundMark(player);
                    player->gainMark("@wrath");

                    room->setCurrent(player);
                    player->play();
                }
            }
            break;
        }

        default:
            break;
        }

        return false;
    }

};

bool HulaoguanScenario::exposeRoles() const{
    return true;
}

void HulaoguanScenario::assign(QStringList &generals, QStringList &roles) const{
    Q_UNUSED(generals);

    roles << "lord";
    int i;
    for(i=0; i<3; i++)
        roles << "rebel";

    qShuffle(roles);
}

int HulaoguanScenario::getPlayerCount() const{
    return 4;
}

void HulaoguanScenario::getRoles(char *roles) const{
    strcpy(roles, "ZFFF");
}

void HulaoguanScenario::onTagSet(Room *room, const QString &key) const{
    // dummy
}

bool HulaoguanScenario::generalSelection() const{
    return true;
}

HulaoguanScenario::HulaoguanScenario()
    :Scenario("hulaoguan_mode")
{
    rule = new HulaoguanRule(this);

    General *godlvbu = new General(this, "godlvbu", "god", 8, true, true);
    godlvbu->addSkill("mashu");
    godlvbu->addSkill("wushuang");

    General *godlvbu2 = new General(this, "godlvbu2", "god", 4, true, true);
    godlvbu2->addSkill("mashu");
    godlvbu2->addSkill("wushuang");
    godlvbu2->addSkill(new Xiuluo);
    godlvbu2->addSkill(new Shenwei);
    godlvbu2->addSkill(new Shenji);

    lord = "godlvbu";

    skills << new ChongzhengSkip << new Chongzhu;

    addMetaObject<ChongzhuCard>();
}

ADD_SCENARIO(Hulaoguan);
