#ifndef HULAOGUANSCENARIO_H
#define HULAOGUANSCENARIO_H

#include "scenario.h"
#include "maneuvering.h"

class HulaoguanScenario : public Scenario{
    Q_OBJECT

public:
    explicit HulaoguanScenario();

    virtual bool exposeRoles() const;
    virtual void assign(QStringList &generals, QStringList &roles) const;
    virtual int getPlayerCount() const;
    virtual void getRoles(char *roles) const;
    virtual void onTagSet(Room *room, const QString &key) const;
    virtual bool generalSelection() const;
};

class ChongzhuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ChongzhuCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

#endif // HULAOGUANSCENARIO_H
