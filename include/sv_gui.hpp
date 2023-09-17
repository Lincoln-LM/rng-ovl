#include <tesla.hpp>
#include "sv_manager.hpp"
#include "species.hpp"
#include "common.hpp"

#ifndef SV_GUI_HPP
#define SV_GUI_HPP

bool isShiny(u32 otid, u32 pid)
{
    u32 temp = otid ^ pid;
    return (u16)(temp ^ (temp >> 16)) < 16;
}

std::string initSpecToString(InitSpec init_spec)
{
    char text[80];
    snprintf(
        text,
        80,
        "%s %s%s",
        init_spec.gender == 2 ? "-" : init_spec.gender == 1 ? "♀"
                                                            : "♂",
        isShiny(init_spec.otid, init_spec.pid) ? "★ " : "",
        SPECIES_LUT[init_spec.species]);
    return std::string(text);
};

class InitSpecGui : public tsl::Gui
{
public:
    InitSpecGui(InitSpec _init_spec)
    {
        init_spec = _init_spec;
    }
    virtual tsl::elm::Element *createUI() override
    {
        char pokemon_text[80];

        sv_manager = SVManager::GetInstance();
        auto frame = new tsl::elm::OverlayFrame(initSpecToString(init_spec), SPECIES_LUT[init_spec.species]);
        auto list = new tsl::elm::List();

        snprintf(pokemon_text, 80, "EC: %08X", init_spec.ec);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "PID: %08X", init_spec.pid);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Ability: %d", init_spec.ability);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Nature: %s", NATURES[init_spec.nature]);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "IVs: %d/%d/%d/%d/%d/%d", init_spec.ivs[0], init_spec.ivs[1], init_spec.ivs[2], init_spec.ivs[3], init_spec.ivs[4], init_spec.ivs[5]);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Scale: %d", init_spec.scale);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        frame->setContent(list);

        return frame;
    }
    virtual void update() override
    {
        sv_manager->update();
    }
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override
    {
        if (keysDown & HidNpadButton_Plus)
        {
            is_focused = !is_focused;
            tsl::hlp::requestForeground(is_focused);
        }
        return !is_focused;
    }

private:
    bool is_focused = true;
    InitSpec init_spec;
    SVManager *sv_manager;
};

class InitSpecItem : public tsl::elm::ListItem
{
public:
    InitSpecItem(InitSpec _init_spec) : ListItem("")
    {
        init_spec = _init_spec;

        setText(initSpecToString(init_spec));
        is_displayed = true;
    }
    virtual bool onClick(u64 keys) override
    {
        if (keys & HidNpadButton_A)
        {
            tsl::changeTo<InitSpecGui>(init_spec);
        }
        return ListItem::onClick(keys);
    }
    InitSpec init_spec;
    bool is_displayed;
};

class SVGui : public tsl::Gui
{
public:
    SVGui() {}
    virtual tsl::elm::Element *createUI() override
    {
        sv_manager = SVManager::GetInstance();
        auto frame = new tsl::elm::OverlayFrame("Scarlet and Violet", "SV");
        list = new tsl::elm::List();
        frame->setContent(list);

        return frame;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override
    {
        if (keysDown & HidNpadButton_Plus)
        {
            is_focused = !is_focused;
            tsl::hlp::requestForeground(is_focused);
        }
        return !is_focused;
    }

    void addPokemon(std::unordered_map<u64, InitSpec> pokemon)
    {
        for (auto &pair : pokemon)
        {
            if (pokemon_items.find(pair.first) == pokemon_items.end())
            {
                auto new_pokemon = new InitSpecItem(pair.second);
                pokemon_items[pair.first] = std::pair<bool, InitSpecItem *>(true, new_pokemon);
                list->addItem(new_pokemon);
            }
            // TODO: despawn/respawn logic as needed
            // else
            // {
            //     auto item = &pokemon_items[pair.first];
            //     if (!pair.second.loaded && item->first)
            //     {
            //         tsl::Gui::removeFocus(item->second);
            //         list->removeItem(item->second);
            //         item->first = false;
            //     }
            //     else if (pair.second.loaded && !item->first)
            //     {
            //         auto new_pokemon = new InitSpecItem(pair.second);
            //         pokemon_items[pair.first] = std::pair<bool, InitSpecItem *>(true, new_pokemon);
            //         list->addItem(new_pokemon);
            //     }
            // }
        }
    }

    virtual void update() override
    {
        sv_manager->update();
        if (!sv_manager->getIsValid())
        {
            // npc_counter->setText("Loaded application is not SV.");
        }
        else
        {
            addPokemon(sv_manager->getPokemon());
        }
    }

private:
    SVManager *sv_manager;
    tsl::elm::List *list;
    std::unordered_map<u64, std::pair<bool, InitSpecItem *>> pokemon_items;
    bool is_focused = true;
};
#endif