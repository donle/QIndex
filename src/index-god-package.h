#ifndef INDEXGODPACKAGE_H
#define INDEXGODPACKAGE_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class IndexGodPackage : public Package{
    Q_OBJECT

public:
   IndexGodPackage();
};

class ShuiyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ShuiyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};
#endif // INDEXGODPACKAGE_H
