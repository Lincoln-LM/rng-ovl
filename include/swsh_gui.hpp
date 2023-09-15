#include <tesla.hpp>
#include "swsh_manager.hpp"
#include "species.hpp"

#ifndef SWSH_GUI_HPP
#define SWSH_GUI_HPP

u64 rotl(u64 x, u64 k)
{
    return (x << k) | (x >> (64 - k));
}

class Xoroshiro
{
public:
    Xoroshiro(u32 seed)
    {
        state[0] = seed;
        state[1] = 0x82A2B175229D6A5B;
    }
    u64 next()
    {
        u64 s0 = state[0];
        u64 s1 = state[1];
        u64 result = s0 + s1;

        s1 ^= s0;
        state[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16);
        state[1] = rotl(s1, 37);

        return result;
    }
    template <u32 max>
    u32 rand()
    {
        auto bit_mask = [](u32 x) constexpr
        {
            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            return x;
        };

        constexpr u32 mask = bit_mask(max);
        if constexpr ((max - 1) == mask)
        {
            return next() & mask;
        }
        else
        {
            u32 result;
            do
            {
                result = next() & mask;
            } while (result >= max);
            return result;
        }
    }

private:
    u64 state[2];
};

char *NATURES[25] = {
    "Hardy",
    "Lonely",
    "Brave",
    "Adamant",
    "Naughty",
    "Bold",
    "Docile",
    "Relaxed",
    "Impish",
    "Lax",
    "Timid",
    "Hasty",
    "Serious",
    "Jolly",
    "Naive",
    "Modest",
    "Mild",
    "Quiet",
    "Bashful",
    "Rash",
    "Calm",
    "Gentle",
    "Sassy",
    "Careful",
    "Quirky"};

std::string pokemonToString(sOverworldPokemon pokemon)
{
    char pokemon_text[80];
    snprintf(
        pokemon_text,
        80,
        "%s %s%s",
        pokemon.generation_data.gender == 1 ? "♂" : "♀",
        pokemon.generation_data.shininess == 1 ? "★ " : "",
        SPECIES_LUT[pokemon.generation_data.species]);
    return std::string(pokemon_text);
};

class OverworldPokemonGui : public tsl::Gui
{
public:
    OverworldPokemonGui(sOverworldPokemon _pokemon)
    {
        pokemon = _pokemon;
    }
    virtual tsl::elm::Element *createUI() override
    {
        char pokemon_text[80];

        swsh_manager = SwShManager::GetInstance();
        auto frame = new tsl::elm::OverlayFrame(pokemonToString(pokemon), SPECIES_LUT[pokemon.generation_data.species]);
        auto list = new tsl::elm::List();
        Xoroshiro rng(pokemon.generation_data.seed);

        snprintf(pokemon_text, 80, "Seed: %08X", pokemon.generation_data.seed);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        u32 ec = rng.next();
        snprintf(pokemon_text, 80, "EC: %08X", ec);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        u32 pid = rng.next();
        u16 tsv = swsh_manager->getTsv();
        bool pid_is_shiny = (((pid >> 16) ^ (pid & 0xFFFF)) ^ tsv) >> 4;
        if (pokemon.generation_data.shininess == 2)
        {
            if (pid_is_shiny)
            {
                pid ^= 0x10000000;
            }
        }
        else
        {
            if (!pid_is_shiny)
            {
                pid = ((tsv ^ (pid & 0xFFFF)) << 16) | (pid & 0xFFFF);
            }
        }
        snprintf(pokemon_text, 80, "PID: %08X", pid);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Ability: %d", pokemon.generation_data.ability);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Nature: %s", NATURES[pokemon.generation_data.nature]);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        u8 ivs[6] = {255, 255, 255, 255, 255, 255};
        for (int i = 0; i < pokemon.generation_data.guaranteed_ivs;)
        {
            int index = rng.rand<6>();
            if (ivs[index] == 255)
            {
                ivs[index] = rng.rand<32>();
                i++;
            }
        }
        for (int i = 0; i < 6; i++)
        {
            if (ivs[i] == 255)
            {
                ivs[i] = rng.rand<32>();
            }
        }
        snprintf(pokemon_text, 80, "IVs: %d/%d/%d/%d/%d/%d", ivs[0], ivs[1], ivs[2], ivs[3], ivs[4], ivs[5]);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        int height = rng.rand<0x81>() + rng.rand<0x80>();
        int weight = rng.rand<0x81>() + rng.rand<0x80>();
        snprintf(pokemon_text, 80, "Height: %d, Weight: %d", height, weight);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        frame->setContent(list);

        return frame;
    }
    virtual void update() override
    {
        swsh_manager->update();
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
    sOverworldPokemon pokemon;
    SwShManager *swsh_manager;
};

class OverworldPokemonItem : public tsl::elm::ListItem
{
public:
    OverworldPokemonItem(sOverworldPokemon _pokemon) : ListItem("")
    {
        pokemon = _pokemon;

        setText(pokemonToString(pokemon));
        is_displayed = true;
    }
    virtual bool onClick(u64 keys) override
    {
        if (keys & HidNpadButton_A)
        {
            tsl::changeTo<OverworldPokemonGui>(pokemon);
        }
        return ListItem::onClick(keys);
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
                    tsl::Gui::removeFocus(item->second);
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
        if (!swsh_manager->getIsValid())
        {
            npc_counter->setText("Loaded application is not SwSh.");
        }
        else
        {
            char npc_text[11];
            snprintf(npc_text, 11, "NPCs: %d", swsh_manager->getNpcCount());
            npc_counter->setText(std::string(npc_text));
            addPokemon(swsh_manager->getOverworldPokemon());
        }
    }

private:
    SwShManager *swsh_manager;
    tsl::elm::List *list;
    tsl::elm::ListItem *npc_counter;
    std::unordered_map<u64, std::pair<bool, OverworldPokemonItem *>> overworld_pokemon_items;
    bool is_focused = true;
};
#endif