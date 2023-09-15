#include <tesla.hpp>
#include "swsh_manager.hpp"
#include "species.hpp"

#ifndef SWSH_GUI_HPP
#define SWSH_GUI_HPP

class OverworldPokemonItem : public tsl::elm::ListItem
{
public:
    OverworldPokemonItem(sOverworldPokemon _pokemon) : ListItem("")
    {
        pokemon = _pokemon;
        char pokemon_text[80];
        snprintf(pokemon_text, 80, "%s, 0x%08X", SPECIES_LUT[pokemon.generation_data.species], pokemon.generation_data.seed);
        setText(std::string(pokemon_text));
        is_displayed = true;
    }
    sOverworldPokemon pokemon;
    bool is_displayed;
};

class SwShGui : public tsl::Gui
{
public:
    SwShGui() {}
    virtual tsl::elm::Element *createUI() override
    {
        swsh_manager = SwShManager::GetInstance();
        auto frame = new tsl::elm::OverlayFrame("Sword and Shield", "SwSh");
        list = new tsl::elm::List();
        npc_counter = new tsl::elm::ListItem("NPCs: ?");
        list->addItem(npc_counter);
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

    void addPokemon(std::unordered_map<u64, sOverworldPokemon> overworld_pokemon)
    {
        for (auto &pair : overworld_pokemon)
        {
            if (overworld_pokemon_items.find(pair.first) == overworld_pokemon_items.end())
            {
                auto new_pokemon = new OverworldPokemonItem(pair.second);
                overworld_pokemon_items[pair.first] = std::pair<bool, OverworldPokemonItem *>(true, new_pokemon);
                list->addItem(new_pokemon);
            }
            else
            {
                auto item = &overworld_pokemon_items[pair.first];
                if (!pair.second.loaded && item->first)
                {
                    list->removeItem(item->second);
                    item->first = false;
                }
                else if (pair.second.loaded && !item->first)
                {
                    auto new_pokemon = new OverworldPokemonItem(pair.second);
                    overworld_pokemon_items[pair.first] = std::pair<bool, OverworldPokemonItem *>(true, new_pokemon);
                    list->addItem(new_pokemon);
                }
            }
        }
    }

    virtual void update() override
    {
        swsh_manager->update();
        char npc_text[11];
        snprintf(npc_text, 11, "NPCs: %d", swsh_manager->getNpcCount());
        npc_counter->setText(std::string(npc_text));
        addPokemon(swsh_manager->getOverworldPokemon());
    }

private:
    SwShManager *swsh_manager;
    tsl::elm::List *list;
    tsl::elm::ListItem *npc_counter;
    std::unordered_map<u64, std::pair<bool, OverworldPokemonItem *>> overworld_pokemon_items;
    bool is_focused = true;
};
#endif