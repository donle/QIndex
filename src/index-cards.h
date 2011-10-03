#ifndef INDEXCARDS_H
#define INDEXCARDS_H

#include "package.h"
#include "card.h"
#include "roomthread.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"

class FreezeSlash: public NatureSlash{
    Q_OBJECT

public:
    Q_INVOKABLE FreezeSlash(Card::Suit suit, int number);
};

class Coin: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Coin(Card::Suit suit, int number);
};

class Arche: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Arche(Card::Suit suit, int number);
};

class SevenMoment: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE SevenMoment(Card::Suit suit, int number);
};

class FoldingFan: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE FoldingFan(Card::Suit suit, int number);

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class Rune: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Rune(Card::Suit suit, int number);
};

class Costume: public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE Costume(Card::Suit suit, int number);
};

class Goggles: public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE Goggles(Card::Suit suit, int number);
};

class Lily: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Lily(Card::Suit suit, int number);
};

class FireSword: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE FireSword(Card::Suit suit, int number);
};

class FireSwordCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE FireSwordCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class Origin: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Origin(Card::Suit suit, int number);
};


class FourArea:public SingleTargetTrick{
    Q_OBJECT

public:
    Q_INVOKABLE FourArea(Card::Suit suit, int number);
    virtual bool isAvailable(const Player *player) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class Recycle:public GlobalEffect{
    Q_OBJECT

public:
    Q_INVOKABLE Recycle(Card::Suit suit, int number);
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};
#endif // INDEXCARDS_H
