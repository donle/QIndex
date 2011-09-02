--qiangzhiyongchang
sgs.ai_skill_invoke.qiangzhiyongchang = function(self, data)
	local players = self.room:getOtherPlayers(self.player)
	local leuges = {}
	for _, player in sgs.qlist(players) do	
		if player:getKingdom() == "shu" then table.insert(leuges, player) end
	end
	
	return #leuges ~= 0
end

sgs.ai_skill_choice.qiangzhiyongchang = function(self, choices)
	local lord = self.room:getLord()
	if self:isFriend(lord) and self.player:getHp() >= 2 then return "yes" end
	
	return "no"
end

--wanquanjiyi
sgs.ai_skill_invoke.wanquanjiyi = true

sgs.ai_skill_playerchosen.wanquanjiyi = function(self, targets)
	for _, target in sgs.qlist(targets) do
		local skills = target:getVisibleSkillList()
		local final_target = true
		for _, skill in sgs.qlist(skills) do
			if self.player:hasSkill(skill:objectName()) then
				final_target = false
				break
			end
		end
		
		if final_target then return target end
	end
	
	return targets[1]
end
	
--huoyanmoshu
sgs.ai_skill_invoke.huoyanmoshu = function(self, data)
	local damage = data:toDamage()
	if damage.to:getArmor() and damage.to:getArmor():objectName() == "vine" and self.player:getHandcardNum() > 0 then return true 
	elseif self.player:getWeapon() and self.player:getWeapon():objectName() == "rune" then return true
	elseif self.player:getHp() >= 3 and self.player:getHandcardNum() > self.player:getHp() then return true
	end
	
	return false
end

sgs.ai_skill_discard.huoyanmoshu = function(self, discard_num, optional, include_equip)
	local flags = "h"
	if include_equip then flags = flags .. "e" end
	local cards = self.player:getCards(flags)
	if cards and cards:length() < discard_num then return {} end
	self:sortByKeepValue(cards, true)
	local discard = {cards[1]:getEffectiveId()}
	
	return discard
end

--linghunranshao
sgs.ai_skill_invoke.linghunranshao = function(self, data)
	local damage = data:toDamage()
	if damage.to:getArmor() and damage.to:getArmor():objectName() == "vine" and self.player:getHp() > 2 then return true
	elseif self.player:getWeapon() and self.player:getWeapon():objectName() == "rune" 
			and self.player:getHp() > 1 and self.player:inMyAttackRange(damage.to) then return true
	elseif damage.to:getHp() == 2 and self.player:getHp() > 1 then return true
	end
	
	return false
end


sgs.ai_skill_choice.chaonenglikaifa = function(self, choices)
	local n = math.random(1, 2)
	if self.player:getRole() == "renegade" then return "routizaisheng"
	else
		return choices:split("+")[n]
	end
end

--beici
local beici_skill={}
beici_skill.name="beicizhiren"
table.insert(sgs.ai_skills,beici_skill)
beici_skill.getTurnUseCard=function(self)
	if self.player:getHandcardNum() <= self.player:getHp() and not self.player:hasUsed("BeiciCard") then
		return sgs.Card_Parse("@BeiciCard=.")
	end
end

sgs.ai_skill_use_func["BeiciCard"]=function(card, use, self)
	local targets = self.room:getOtherPlayers(self.player)
	local target
	local target_num = 0
	for _, player in sgs.qlist(targets) do
		if self:isEnemy(player) then
			if player:getHandcardNum() > target_num then
				target = player
				target_num = player:getHandcardNum()
			end
		end
	end
	
	if target then 
		if use.to then
			use.to:append(target)
		end
		use.card = card
	end
end

sgs.ai_skill_ag.beicizhiren = function(self, card_ids, refusable)
	local cards = {}
	for _, card_id in sgs.qlist(card_ids) do
		local card = sgs.Sanguosha:getCard(card_id)
		table.insert(cards, card)
	end
	
	self:sortByKeepValue(cards, true)
	
	return cards[1]:getEffectiveId()
end

--lingnenglizhe
local shangtiao_ai = SmartAI:newSubclass("shangtiaodangma")

function shangtiao_ai:askForCard(pattern, prompt)
	local card = super.askForCard(self, pattern, prompt)
	if card then return card end
	if pattern == "slash" then
		local cards = self.player:getCards("h")
		cards=sgs.QList2Table(cards)
		self:fillSkillCards(cards)
		for _, card in ipairs(cards) do
			if card:inherits("Weapon") then
				local suit = card:getSuitString()
				local number = card:getNumberString()
				local card_id = card:getEffectiveId()
				return ("zero_slash:wushen[%s:%s]=%d"):format(suit, number, card_id)
			end
		end
	end
end

--huanxiangshashou
sgs.ai_skill_invoke.huanxiangshashou = function(self, data)
	return not self.player:isKongcheng()
end

sgs.ai_skill_discard.huanxiangshashou = function(self, discard_num, optional, include_equip)
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	local discard = {}
	table.insert(discard, cards[1]:getEffectiveId())
	return discard
end

--xixueshashou
sgs.ai_skill_invoke.xixueshashou = function(self, data)
	local recover = data:toRecover()
	if self:isEnemy(recover.who) and recover.who:getHp() <= 0 then return true end
	return false
end

sgs.ai_skill_discard.xixueshashou = sgs.ai_skill_discard.huanxiangshashou

--chaodiancipao
sgs.ai_skill_invoke.chaodiancipao = function(self, data)
	local players = self.room:getOtherPlayers(self.player)
	local targets = {}
	for _, player in sgs.qlist(players) do
		if self.player:distanceTo(player) <= 1 and self:isFriend(player) then table.insert(targets, player) end
	end
	
	return #targets ~= 0
end

sgs.ai_skill_playerchosen.chaodiancipao = function(self, targets)
	local target
	local target_num = 0
	for _, t in sgs.qlist(targets) do
		if self:isEnemy(t) and t:getHandcardNum() > target_num then
			target = t
			target_num = t:getHandcardNum()
		end
	end
	
	if target then
		return target
	else 
		return targets:at(0)
	end
end

--xinlingzhangwo
sgs.ai_skill_invoke.xinlingzhangwo = function(self, data)
	local targets = {}
	for _, player in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if self.player:distanceTo(player) <= 2 and self:isEnemy(player) then
			table.insert(targets, player)
		end
	end
	
	return #targets ~= 0
end

sgs.ai_skill_playerchosen.xinlingzhangwo = function(self, targets)
	for _, target in sgs.qlist(targets) do
		if self:isEnemy(target) then return target end
	end
	
	return targets:at(0)
end

--kongjianyidong
sgs.ai_skill_invoke.kongjianyidong = function(self, data)
	if self:isFriend(self.player:getNextAlive()) and self.player:getHp() > self.player:getNextAlive() then return false
	else return true
	end
end

--shiliangcaozong
sgs.ai_skill_invoke.shiliangcaozong = function(self, data)
	local damage = data:toDamage()
	if self:isFriend(damage.from) and damage.from:getHp() <= 2 and self.player:getHp() >= 2 then return false
	else return true end
end

sgs.ai_skill_discard.shiliangcaozong =  function(self, discard_num, optional, include_equip)
	local equips = self.player:getCards("e")
	local cards = self.player:getHandcards()
	local discard = {}
	if equips then table.insert(discard, equips:at(equips:length()-1))
	else
		cards = sgs.QList2Table(cards)
		self:sortByUseValue(cards, true)
		table.insert(discard, cards[1]:getEffectiveId())
	end
	return discard
end

--yifangtongxing
sgs.ai_skill_invoke.yifangtongxing = true

sgs.ai_skill_choice.yifangtongxing = function(self, choices)
	local lord = self.room:getLord()
	if self:isFriend(lord) then
		if lord:getHp() == 2 and self.player:getHp() > 2 then return "yes"
		elseif lord:getHp() == 1 and lord:isKongcheng() and self.player:getRole() == "loyalist" then return "yes"
		end
	end
	return "no"
end

--dingwenbaocun
local baocun_skill={}
baocun_skill.name="dingwenbaocun"
table.insert(sgs.ai_skills,baocun_skill)
baocun_skill.getTurnUseCard=function(self)
	if self.dingwenbaocun_used then return end
	
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards)
	for _, card in ipairs(cards) do	
		if not (card:inherits("Jink") or card:inherits("Nullification") or card:getTypeId() == sgs.Card_Equip or card:inherits("DelayedTrick")) then
			if not self.player:isWounded() and card:inherits("Peach") then local shit = nil
			else
				local suit = card:getSuit()
				local number = card:getNumber()
				local card_clone = sgs.Sanguosha:cloneCard(card:objectName(), suit, number)
				card_clone:setSkillName("dingwenbaocun")
				self.dingwenbaocun_used = true
				return card_clone
			end
		end
	end
end

--shouhuzhishen
sgs.ai_skill_invoke.shouhuzhishen = function(self, data)
	return #self.friends_noself ~= 0
end

sgs.ai_skill_playerchosen.shouhuzhishen = function(self, targets)
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends_noself) do
		if friend ~= self.player then return friend end
	end
	
	return targets:at(0)
end

--huanxiangyushou
sgs.ai_skill_invoke.huanxiangyushou = function(self, data)
	return self.player:isWounded()
end

sgs.ai_skill_playerchosen.huanxiangyushou = function(self, targets)
	for _, target in sgs.qlist(targets) do
		if self:isEnemy(target) then return target end
	end
	
	return targets:at(0)
end

--dakonglishi
sgs.ai_skill_invoke.dakonglishi = function(self, data)
	local effect = data:toSlashEffect()
	if self:isFriend(effect.to) or effect.to:getHp() > 2 then return false
	else return true
	end
end

--nianlibaopo
sgs.ai_skill_invoke.nianlibaopo = function(self, data)
	local damage = data:toDamage()
	return self:isEnemy(damage.to:getNextAlive()) or damage.to:getNextAlive():getHp() > 2
end

--zuobiaoyidong
sgs.ai_skill_invoke.zuobiaoyidong = function(self, data)
	local effect = data:toCardEffect()
	return self:isEnemy(effect.from)
end

--renzaotianshi
sgs.ai_skill_playerchosen.renzaotianshi = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, target in ipairs(targets) do
		if self:isFriend(target) then return target end
	end
	
	return targets[1]
end

--zhengtibuming
sgs.ai_skill_choice.zhengtibuming = function(self, choices)
	local choice = choices:split("+")
	local n = math.random(1, 3)
	return choice[n]
end

--cilixuji
sgs.ai_skill_invoke.cilixuji = function(self, data)
	local effect = data:toSlashEffect()
	return self:getJinkNumber(effect.to) == 0 
end

--jinxingguangxian
local jinxing_skill={}
jinxing_skill.name="jinxingguangxian"
table.insert(sgs.ai_skills, jinxing_skill)
jinxing_skill.getTurnUseCard=function(self)
	local has_weapon 
	local card_str
	if self.player:getWeapon() then has_weapon = true end
	local cards = self.player:getHandcards()
	for _, card in sgs.qlist(cards) do
		if card:inherits("Weapon") then
			card_str = "@JinxingCard=" .. card:getEffeciveId() 
			break
		end
	end
	if card_str then
		return sgs.Card_Parse(card_str)
	elseif has_weapon and self.player:getOffensiveHorse() then
		return sgs.Card_Parse("@JinxingCard=" .. self.player:getWeapon():getEffectiveId())
	end
end

sgs.ai_skill_use_func["JinxingCard"] = function(card, use, self)
	local target
	for _, enemy in ipairs(self.enemies) do	
		if not enemy:getCards("hej") then
			target = enemy
			break
		elseif enemy:getHp() >= enemy:getCards("hej"):length() then
			if not target then
				target = enemy
			else
				if target:getCards("hej"):length() > enemy:getCards("hej"):length() then
					target = enemy
				end
			end
		end
	end
	
	if not target then 
		for _, friend in ipairs(self.friends_noself) do
			if friend:getCards("j") then
				if not friend then target = friend 
				else	
					if target:getCards("e") and target:getCards("e"):length() < friend:getCards("e"):length() then
						target = friend
					end
				end
			elseif not friend:getCards("e") then 
				if not friend then target = friend
				else
					if target:getHandcardNum() < friend:getHandcardNum() then
						target = friend
					end
				end
			end
		end
	end
	
	if target then
		if use.to then	
			use.to:append(target)
		end
		use.card = card
	end
end

--tuolizhiqiang
sgs.ai_skill_invoke.kewutuolizhiqiang = true

sgs.ai_skill_playerchosen.kewutuolizhiqiang = function(self, targets)
	local target
	local target_hp = 0
	for _, player in sgs.qlist(targets) do	
		if self:isEnemy(player) and player:getHp() > target_hp() then
			target = player
			target_hp = player:getHp()
		end
	end
	return target
end

--diwuyuansu
sgs.ai_get_cardType=function(card)
	if card:inherits("Weapon") then return 1 end
	if card:inherits("Armor") then return 2 end 
	if card:inherits("OffensiveHorse")then return 3 end 
	if card:inherits("DefensiveHorse") then return 4 end 
end

local five_skill={}
five_skill.name="diwuyuansu"
table.insert(sgs.ai_skills, five_skill)
five_skill.getTurnUseCard=function(self)
	local equips = self.player:getCards("e")
	if not equips then return end
	
	local eCard
	local hasCard={0, 0, 0, 0}
	for _, card in sgs.qlist(self.player:getCards("he")) do
		if card:inherits("EquipCard") then 
			hasCard[sgs.ai_get_cardType(card)] = hasCard[sgs.ai_get_cardType(card)]+1
		end		
	end
	
	for _, card in sgs.qlist(equips) do
		if hasCard[sgs.ai_get_cardType(card)]>1 or sgs.ai_get_cardType(card)>3 then 
			eCard = card 
			break
		end
		if not eCard and not card:inherits("Armor") then eCard = card end
	end
	if not eCard then return end
	
	local suit = eCard:getSuitString()
	local number = eCard:getNumberString()
	local card_id = eCard:getEffectiveId()
	local card_str = ("duel:diwuyuansu[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)
	
    assert(skillcard)
    return skillcard
end

--baiyi
local baiyi_skill={}
baiyi_skill.name="baiyi"
table.insert(sgs.ai_skills, baiyi_skill)
baiyi_skill.getTurnUseCard=function(self)
	if self.player:getMark("@wings") <= 0 or not self:slashIsAvailable() then return end
	
	local players = self.room:getOtherPlayers(self.player)
	local targets = {}
	for _, player in sgs.qlist(players) do
		if self:isEnemy(player) and not self.player:inMyAttackRange(player) and player:getJinkNumber() == 0 then
			table.insert(targets, player)
		end
	end
	if #targets == 0 or self.player:getSlashNumber() == 0 then return end
	return sgs.Card_Parse("@BaiyiCard=" .. self:getSlash():getEffectiveId())	
end

sgs.ai_skill_use_func["BaiyiCard"] = function(card, use, self)
	local players = self.room:getOtherPlayers(self.player)
	local targets = {}
	for _, player in sgs.qlist(players) do
		if self:isEnemy(player) and not self.player:inMyAttackRange(player) and player:getJinkNumber() == 0 then
			table.insert(targets, player)
		end
	end
	
	self:sort(targets, "hp")
	for _, target in ipairs(targets) do
		if target:getJinkNumber() == 0 and self:slashIsEffective(self:getSlash(), target) then
			if use.to then use.to:append(target) end
			use.card = card
			return
		end
	end
	
	if use.to then use.to:append(targets[1]) end
	use.card = card
end

--weiyuanwuzhi
sgs.ai_skill_choice.weiyuanwuzhi = function(self, choices)
	if self:getMark("@wings") <= 2 then return "getmark" end
	if self.player:getHp() >= self.player:getHandcardNum() then return "exnihilo" end
	
	return "getmark"
end	

--yuanzibenghuai
sgs.ai_skill_invoke.yuanzibenghuai = function(self, data)
	local effect = data:toCardEffect()
	return self:isEnemy(effect.from)
end

--lizishexian
sgs.ai_skill_invoke.liziboxinggaosupao = function(self, data)
	local damage = data:toDamage()
	return self:isEnemy(damage.to)
end

--danqizhuangjia
sgs.ai_skill_invoke.danqizhuangjia = function(self, data)
	local damage = data:toDamage()
	if damage.to == self.player then 
		if damage.damage == 1 and (self:getPeachNum() > 0 or self.player:getHp() > 2) then return false
		else
			return true
		end
	elseif damage.from == self.player then
		if (damage.card:inherits("FireAttack") or damage.card:inherits("FireSlash")) and damage.to:getArmor() and damage.to:getArmor():objectName() == "vine" then
			return true
		elseif damage.damage > 1 then return true
		elseif self.player:getPile("nitro") and self.player:getPile("nitro"):length() > 2 then return true
		else
			return false
		end
	end
	
	return true
end

