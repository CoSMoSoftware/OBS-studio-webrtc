package io.cosmosoftware.kite.obs.steps;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.OutputSettingPage;
import io.cosmosoftware.kite.steps.TestStep;
import io.cosmosoftware.kite.util.ReportUtils;

public class ChangeBitrateStep extends TestStep {

  private final MainPage mainPage;
  private final OutputSettingPage outputSettingPage;
  private final String bitrate;


  public ChangeBitrateStep(Runner runner, String bitrate) {
    super(runner);
    this.mainPage = new MainPage(runner);
    this.outputSettingPage = new OutputSettingPage(runner);
    this.bitrate = bitrate;
  }

  @Override
  protected void step() throws KiteTestException {
    mainPage.openSettings();
    try {
      outputSettingPage.openSettingOption();
      outputSettingPage.changeVideoBitrate(this.bitrate);
      this.reporter.screenshotAttachment(this.report, this.getClassName(), ReportUtils
          .saveScreenshotPNG(this.webDriver));
    } catch (Exception e) {
      throw e;
    } finally {
      outputSettingPage.applyChange();
    }
  }

  @Override
  public String stepDescription() {
    return "Change bitrate to : " + this.bitrate + " Kbps";
  }

}
