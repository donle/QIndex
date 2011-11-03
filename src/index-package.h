#ifndef QINGJIAOPACKAGE_H
#define QINGJIAOPACKAGE_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class IndexPackage : public Package{
    Q_OBJECT

public:
   IndexPackage();
   void addCards();
};

class JiyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JiyiCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class ZeroSlash: public Slash{
    Q_OBJECT

public:
    Q_INVOKABLE ZeroSlash(Card::Suit suit, int number);

    virtual void setNature(DamageStruct::Nature nature);
};

class WeishanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE WeishanCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class BaiyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BaiyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class BeiciCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BeiciCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class JinxingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JinxingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class ShenyinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ShenyinCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class ShendianCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ShendianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class BaoqiangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BaoqiangCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class ShufuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ShufuCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ZhuijiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuijiCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DingguiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE DingguiCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class WuzhengCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE WuzhengCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JiushuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JiushuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DayanshuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE DayanshuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SuqingCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SuqingCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class WangluoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE WangluoCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class GetiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE GetiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class HunluanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE HunluanCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class BenghuaiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE BenghuaiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

class QizuiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE QizuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};
#endif // QINGJIAOPACKAGE_H
