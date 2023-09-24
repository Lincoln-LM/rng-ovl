#include <tesla.hpp>
#include "swsh_manager.hpp"
#include "species.hpp"
#include "weather.hpp"
#include "overworld_pokemon_gui.hpp"

#ifndef SWSH_GUI_HPP
#define SWSH_GUI_HPP

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
        weather_display = new tsl::elm::ListItem("Weather: ?");
        list->addItem(weather_display);
        rain_calibration_display = new tsl::elm::ListItem("Rain Calibration: ?");
        rain_calibration_display->setClickListener([this](u64 keys)
                                                   {
            if (!is_focused) {
                return false;
            }
            if (!(keys & HidNpadButton_A)) {
                return false;
            }
            swsh_manager->startRainCalibration();
            
            return true; });
        list->addItem(rain_calibration_display);
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
            char weather_text[30];
            Weather weather = swsh_manager->getWeather();
            snprintf(weather_text, 30, "Weather: %s", weather == Weather::INVALID ? "?" : WEATHER_LUT[weather].c_str());
            weather_display->setText(std::string(weather_text));
            char rain_calibration_text[30];
            snprintf(rain_calibration_text, 30, "Rain Calibration: %d", swsh_manager->getRainCalibration());
            rain_calibration_display->setText(std::string(rain_calibration_text));
            addPokemon(swsh_manager->getOverworldPokemon());
        }
    }

private:
    SwShManager *swsh_manager;
    tsl::elm::List *list;
    tsl::elm::ListItem *npc_counter;
    tsl::elm::ListItem *weather_display;
    tsl::elm::ListItem *rain_calibration_display;
    std::unordered_map<u64, std::pair<bool, OverworldPokemonItem *>> overworld_pokemon_items;
    bool is_focused = true;
};
#endif