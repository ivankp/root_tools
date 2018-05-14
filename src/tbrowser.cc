#include <iostream>
#include <list>

#include <TApplication.h>
#include <TBrowser.h>
#include <TFile.h>
#include <TStyle.h>

int main(int argc, char* argv[])
{
  int app_argc = 1;
  char app_argv1[] = "l";
  char *app_argv[] = { app_argv1 };
  TApplication app("App",&app_argc,app_argv);

  gStyle->SetOptStat(110010);
  gStyle->SetPaintTextFormat(".2f");

  std::list<TFile> files;
  for (int i=1; i<argc; ++i)
    files.emplace_back(argv[i]);

  TBrowser b;

  app.Run();
  return 0;
}
