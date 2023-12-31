#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <memory>
#include "debug_handler.hpp"
#include "swsh.hpp"

class MainGui : public tsl::Gui
{
public:
    MainGui() {}

    virtual tsl::elm::Element *createUI() override
    {
        debug_handler = DebugHandler::GetInstance();

        auto frame = new tsl::elm::OverlayFrame("RNG Overlay", "v0.0.3");

        auto list = new tsl::elm::List();
        attach_button = new tsl::elm::ListItem("Attach...");
        attach_button->setClickListener([this](u64 keys)
                                        {
            if (!is_focused) {
                return false;
            }
            if (!(keys & HidNpadButton_A)) {
                return false;
            }
            if (debug_handler->isAttached()) {
                debug_handler->Detach();
                return true;
            }

            bool succeeded = R_SUCCEEDED(debug_handler->Attach());
            if (succeeded) {
                debug_handler->Continue();
            }
            
            return succeeded; });

        list->addItem(attach_button);
        swsh_button = new tsl::elm::ListItem("SwSh");
        swsh_button->setClickListener([this](u64 keys)
                                      { 
            if (keys & HidNpadButton_A) {
                tsl::changeTo<SwShGui>();
                return true;
            } 
            return false; });
        list->addItem(swsh_button);
        frame->setContent(list);

        return frame;
    }

    virtual void update() override
    {
        if (debug_handler->isAttached())
        {
            attach_button->setText("Detach");
            if (!debug_handler->isBroken())
            {
                debug_handler->Poll();
            }
        }
        else
        {
            attach_button->setText("Attach...");
        }
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
    DebugHandler *debug_handler;

    tsl::elm::ListItem *attach_button;
    tsl::elm::ListItem *swsh_button;

    bool is_focused = true;
};

class RNGOverlay : public tsl::Overlay
{
public:
    virtual void initServices() override
    {
        Result rc = ldrDmntInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
        rc = pminfoInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
    }
    virtual void exitServices() override
    {
        DebugHandler::GetInstance()->Detach();
        ldrDmntExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override
    {
        return initially<MainGui>();
    }
};

int main(int argc, char **argv)
{
    return tsl::loop<RNGOverlay>(argc, argv);
}
