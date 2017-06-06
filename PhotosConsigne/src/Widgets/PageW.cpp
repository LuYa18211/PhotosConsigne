

#include "PageW.hpp"

static int pageUICounter = 0;

pc::PageW::PageW() : SettingsW(){

    ui.setupUi(this);
    ui.framePage->setEnabled(false);
    ui.vlBackground->addWidget(&backgroundW);
    ui.vlBorders->addWidget(&bordersW);
    ui.vlMargins->addWidget(&marginsW);
    ui.vlSets->addWidget(&setsW);
    ui.vlMisc->addWidget(&miscW);
    backgroundW.hide();
    bordersW.hide();
    marginsW.hide();
    miscW.hide();


    init_checkboxes_connections({ui.cbEnableIndividualPage}, true);
    Utility::checkbox_enable_UI(ui.cbEnableIndividualPage, {ui.framePage});

    connect(ui.tabPage, &QTabWidget::currentChanged, this, [&]{

        backgroundW.hide();
        bordersW.hide();
        marginsW.hide();
        setsW.hide();
        miscW.hide();

        switch (ui.tabPage->currentIndex()) {
        case 0:
            setsW.show();
            break;
        case 1:
            marginsW.show();
            break;
        case 2:
            bordersW.show();
            break;
        case 3:
            backgroundW.show();
            break;
        case 4:
            miscW.show();
            break;
        }
    });

//     # background
    connect(&backgroundW, &BackgroundW::settings_updated_signal, this, &PageW::settings_updated_signal);
//     # borders
    connect(&bordersW, &BordersW::settings_updated_signal, this, &PageW::settings_updated_signal);
//     # margins
    connect(&marginsW, &MarginsW::settings_updated_signal, this, &PageW::settings_updated_signal);
//     # sets
    connect(&setsW, &PageSetsW::settings_updated_signal, this, &PageW::settings_updated_signal);
//     # misc
    connect(&miscW, &MiscPageW::settings_updated_signal, this, &PageW::settings_updated_signal);

    id = pageUICounter++;
}


void pc::PageW::init_ui(pc::PageW &p1, const pc::PageW &p2){

    // init ui with
    PageSetsW::init_ui(p1.setsW, p2.setsW);
    BackgroundW::init_ui(p1.backgroundW, p2.backgroundW);
    BordersW::init_ui(p1.bordersW, p2.bordersW);
    MarginsW::init_ui(p1.marginsW, p2.marginsW);
    MiscPageW::init_ui(p1.miscW, p2.miscW);
}

bool pc::PageW::individual() const noexcept{
    return ui.cbEnableIndividualPage->isChecked();
}

void pc::PageW::set_enabled(bool enabled) noexcept{
    Utility::safe_init_checkboxe_checked_state(ui.cbEnableIndividualPage, enabled);
    ui.framePage->setEnabled(enabled);
}

