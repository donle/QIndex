#include "index-cards.h"
#include "index-package.h"
#include "client.h"
#include "engine.h"
#include "carditem.h"

class CoinSkill: public WeaponSkill{
public:
    CoinSkill():WeaponSkill("coin"){
        events << Predamage;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Thunder){
            LogMessage log;
            log.type = "#CoinEffect";
            log.from = player;
            room->sendLog(log);

            damage.nature = DamageStruct::Thunder;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

Coin::Coin(Suit suit, int number):Weapon(suit, number, 5){
    setObjectName("coin");
    skill = new CoinSkill;
}

class ArcheSkill: public WeaponSkill{
public:
    ArcheSkill():WeaponSkill("arche"){
        events << FinishJudge;
        frequency = Compulsory;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getWeapon() && target->getWeapon()->objectName() == objectName();
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        Room *room = player->getRoom();
        if(room->getCurrent() != player)
            return false;

        if(judge->card->getSuit() == Card::Diamond){
            LogMessage log;
            log.type = "#ArcheEffect";
            log.from = player;
            room->sendLog(log);

            Card *new_card = Card::Clone(judge->card);
            new_card->setSuit(Card::Club);
            judge->card = new_card;
        }
        return false;
    }
};

Arche::Arche(Suit suit, int number):Weapon(suit, number, 5){
    setObjectName("arche");
    skill = new ArcheSkill;
}

class SevenMomentSkill: public WeaponSkill{
public:
    SevenMomentSkill():WeaponSkill("seven_moment"){
        events << SlashEffect;
        frequency = Compulsory;
    }

    virtual int getPriority() const{
     return -1;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        Room *room = player->getRoom();
        if(!player->isWounded())
            return false;

        LogMessage log;
        log.type = "#SevenMomentEffect";
        log.from = player;
        room->sendLog(log);

        if(!room->askForCard(effect.to, "jink", "@sevenmoment-slash:"+player->objectName())){
            room->slashResult(effect, NULL);
            return true;
        }

        return false;
    }
};

SevenMoment::SevenMoment(Suit suit, int number):Weapon(suit, number, 3){
    setObjectName("seven_moment");
    skill = new SevenMomentSkill;
}


FoldingFan::FoldingFan(Card::Suit suit, int number):Weapon(suit, number, 2){
    setObjectName("folding_fan");
}

void FoldingFan::onInstall(ServerPlayer *player) const{
    Room *room = player->getRoom();
    room->setPlayerProperty(player, "atk", range);

    LogMessage log;
    log.type = "#FoldingFanGood";
    log.from = player;
    room->sendLog(log);

    room->setPlayerProperty(player, "maxhp", player->getMaxHP()+1);
    room->setPlayerProperty(player, "hp", player->getHp()+1);
}

void FoldingFan::onUninstall(ServerPlayer *player) const{
    Room *room = player->getRoom();
    room->setPlayerProperty(player, "atk", 1);
    LogMessage log;
    log.type = "#FoldingFanBad";
    log.from = player;
    room->sendLog(log);

    if(player->isAlive())
        room->setPlayerProperty(player, "hp", player->getHp()-1);

    room->setPlayerProperty(player, "maxhp", player->getMaxHP()-1);

    if(player->getHp() <= 0 && player->isAlive() && !player->hasFlag("net"))
        room->enterDying(player, NULL);
}

class RuneSkill: public WeaponSkill{
public:
    RuneSkill():WeaponSkill("rune"){
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature != DamageStruct::Fire || !player->isWounded() || !player->inMyAttackRange(damage.to))
            return false;

        LogMessage log;
        log.type = "#RuneEffect";
        log.from = player;
        room->sendLog(log);

        RecoverStruct recover;
        recover.who = player;
        room->recover(player, recover);

        return false;
    }
};

Rune::Rune(Card::Suit suit, int number):Weapon(suit, number, 2){
    setObjectName("rune");
    skill = new RuneSkill;
}

class CostumeSkill: public ArmorSkill{
public:
    CostumeSkill():ArmorSkill("costume"){
        events << Predamaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        Room *room = player->getRoom();

        if(damage.nature != DamageStruct::Normal){
            LogMessage log;
            log.type = "#CostumeEffect";
            log.from = player;
            room->sendLog(log);

            return true;
        }

        return false;
    }
};

Costume::Costume(Card::Suit suit, int number):Armor(suit, number){
    setObjectName("costume");
    skill = new CostumeSkill;
}

class LilySkill: public WeaponSkill{
public:
    LilySkill():WeaponSkill("lily"){
        events << CardUsed;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(!use.card->inherits("Duel"))
            return false;

        player->drawCards(1);
        return false;
    }
};

Lily::Lily(Card::Suit suit, int number):Weapon(suit, number, 2){
    setObjectName("lily");
    skill = new LilySkill;
}

FireSwordCard::FireSwordCard(){
    once = true;
}

bool FireSwordCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return Self->canSlash(to_select) && targets.length() < 3;
}

void FireSwordCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);
    room->loseHp(source);

    int card_id = subcards.first();
    const Card *slash = Sanguosha->getCard(card_id);

    bool can_damage = false;
    foreach(ServerPlayer *p, targets){
        const Card *jink = room->askForCard(p, "jink", "firesword-slash:"+source->objectName());
        if(!jink){
            can_damage = true;
            break;
        }
    }

    foreach(ServerPlayer *p, targets){
        if(can_damage){
            DamageStruct damage;
            damage.card = slash;
            damage.damage = 1;
            damage.nature = DamageStruct::Fire;
            damage.from = source;
            damage.to = p;
            room->damage(damage);
        }
    }

}

class FireSwordSkill: public OneCardViewAsSkill{
public:
    FireSwordSkill():OneCardViewAsSkill("fire_sword"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("FireSwordCard") && Slash::IsAvailable(player);
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();
        return card->inherits("ThunderSlash") || card->inherits("FireSlash");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        FireSwordCard *card = new FireSwordCard;
        card->addSubcard(card_item->getCard());
        return card;
    }
};

FireSword::FireSword(Card::Suit suit, int number):Weapon(suit, number, 3){
    setObjectName("fire_sword");
    attach_skill = true;
}

class OriginSkill: public TriggerSkill{
public:
    OriginSkill():TriggerSkill("origin"){
        events << CardResponsed << CardUsed;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasWeapon(objectName());
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::NotActive)
            return false;

        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(!use.card->inherits("Nullification"))
                return false;
        }
        player->drawCards(1);

        return false;
    }
};

Origin::Origin(Card::Suit suit, int number):Weapon(suit, number, 2){
    setObjectName("origin");
    skill = new OriginSkill;
}

FourArea::FourArea(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("four_area");
}

bool FourArea::isAvailable(const Player *player) const{
    QList<const Player*> players = player->parent()->findChildren<const Player *>();
    foreach(const Player *p, players){
        if(player->inMyAttackRange(p))
            return true;
    }

    return false;
}

bool FourArea::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

bool FourArea::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.isEmpty())
        return Self->inMyAttackRange(to_select) && Self != to_select;
    else if(targets.length() == 1)
        return targets.first()->inMyAttackRange(to_select);
    else
        return false;
}

void FourArea::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->throwCard(this);

    targets.at(0)->setTempData(targets.at(1), this->getEffectiveId());
    targets.at(1)->setTempData(targets.at(0), this->getEffectiveId());
    targets.at(1)->setMark("second_target", 1);

    bool on_effect = room->cardEffect(this, source, targets.at(0));
    if(on_effect)
        room->showAllCards(targets.at(0), source);

    on_effect = room->cardEffect(this, targets.at(0), targets.at(1));
    if(on_effect)
        room->showAllCards(targets.at(1), targets.at(0));
}

Recycle::Recycle(Card::Suit suit, int number)
    :GlobalEffect(suit, number)
{
    setObjectName("recycle");
}

void Recycle::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->throwCard(this);

    const Card *show = room->askForCard(source, ".", "@recycle-show", false);
    if(show == NULL)
        return;

    switch(show->getTypeId()){
    case Card::Basic:{
            room->setTag("recycle", "basic");
            break;
        }
    case Card::Trick:{
            room->setTag("recycle", "trick");
            break;
        }
    case Card::Equip:{
            room->setTag("recycle", "equip");
            break;
        }
    default:
        break;
    }

    room->showCard(source, show->getEffectiveId());
    GlobalEffect::use(room, source, room->getOtherPlayers(source));
}

void Recycle::onEffect(const CardEffectStruct &effect) const{
    if(effect.from == effect.to)
        return;

    Room *room = effect.from->getRoom();

    QString pattern = room->getTag("recycle").toString();

    const Card *card = room->askForCard(effect.to, pattern, "@recycle-choice", false);
    if(card == NULL){
        LogMessage log;
        log.type = "#RecycleFail";
        log.from = effect.from;
        log.to << effect.to;
        room->sendLog(log);

        effect.to->throwAllHandCards();
    }
    else{
        room->throwCard(card->getEffectiveId(), true);
    }
}

void IndexPackage::addCards(){
    QList<Card *> cards;

    cards << new FourArea(Card::Spade, 13)
          << new FourArea(Card::Club, 13)
          << new FourArea(Card::Diamond, 13)
          << new FourArea(Card::Heart, 13)
          << new Recycle(Card::Spade, 2)
          << new Recycle(Card::Heart, 2);

    cards << new Coin(Card::Spade, 1)
          << new Arche(Card::Club, 1)
          << new SevenMoment(Card::Heart, 1)
          << new Rune(Card::Diamond, 1)
          << new FoldingFan(Card::Club, 10)
          << new Lily(Card::Club, 9)
          << new FireSword(Card::Heart, 12)
          << new Origin(Card::Club, 3);

    cards << new Costume(Card::Club, 7);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;

    skills << new FireSwordSkill;

    addMetaObject<FireSwordCard>();
}
