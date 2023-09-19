#include <tesla.hpp>
#include "swsh_manager.hpp"
#include "overworld_pokemon.hpp"

#ifndef OVERWORLD_POKEMON_GUI_HPP
#define OVERWORLD_POKEMON_GUI_HPP

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
        auto frame = new tsl::elm::OverlayFrame(pokemon.toString(), pokemon.speciesName().c_str());
        auto list = new tsl::elm::List();

        snprintf(pokemon_text, 80, "Seed: %08X", pokemon.init_spec.seed);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "EC: %08X", pokemon.fixed_info.ec);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "PID: %08X", pokemon.fixed_info.pid);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Ability: %d", pokemon.init_spec.ability);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Nature: %s", pokemon.natureName().c_str());
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(
            pokemon_text,
            80,
            "IVs: %d/%d/%d/%d/%d/%d",
            pokemon.fixed_info.ivs[0],
            pokemon.fixed_info.ivs[1],
            pokemon.fixed_info.ivs[2],
            pokemon.fixed_info.ivs[3],
            pokemon.fixed_info.ivs[4],
            pokemon.fixed_info.ivs[5]);
        list->addItem(new tsl::elm::ListItem(std::string(pokemon_text)));

        snprintf(pokemon_text, 80, "Height: %d, Weight: %d", pokemon.fixed_info.height, pokemon.fixed_info.weight);
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
    OverworldPokemonItem(sOverworldPokemon pokemon) : ListItem("")
    {
        m_pokemon = pokemon;

        setText(m_pokemon.toString());
    }

private:
    virtual bool onClick(u64 keys) override
    {
        if (keys & HidNpadButton_A)
        {
            tsl::changeTo<OverworldPokemonGui>(m_pokemon);
        }
        return ListItem::onClick(keys);
    }

    sOverworldPokemon m_pokemon;
};

#endif