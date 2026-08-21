#pragma once

#include <borealis.hpp>
#include <borealis/core/task.hpp>
#include <atomic>
#include <memory>

class MainActivity : public brls::Activity
{
  public:
    CONTENT_FROM_XML_RES("activity/main.xml");

    void onContentAvailable() override;

  private:
    brls::Label* wifiIcon   = nullptr;
    brls::Label* wifiStatus = nullptr;

    // Spinner de loading
    class SpinnerTask : public brls::RepeatingTask
    {
      public:
        SpinnerTask(brls::Label* icon) : brls::RepeatingTask(120), icon(icon) {}
        void run() override
        {
            static const char* frames[] = { "|", "/", "-", "\\" };
            icon->setText(frames[frame % 4]);
            frame++;
        }
        brls::Label* icon;
        int frame = 0;
    };

    SpinnerTask* spinner = nullptr;

    void checkConnection();
};
