#pragma once

#include <borealis.hpp>

class MainActivity : public brls::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/main.xml");

    void onContentAvailable() override;

  private:
    brls::Label* wifiIcon = nullptr;
    brls::Label* wifiStatus = nullptr;

    void checkConnection();
};
