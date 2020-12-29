package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.OutputSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;

public class OutputAudioBitrateEnableCheck extends TestCheck {

  private final MainPage mainPage;
  private final OutputSettingPage outputSettingPage;

  public OutputAudioBitrateEnableCheck(Runner runner) {
    super(runner);
    this.setOptional(true);
    this.mainPage = new MainPage(runner);
    this.outputSettingPage = new OutputSettingPage(runner);
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.outputSettingPage.openSettingOption();
    boolean audioBitrateEnable = this.outputSettingPage.audioBitrateEnabled();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.outputSettingPage.discardChange();
    if (!audioBitrateEnable) {
      throw new KiteTestException(
          "The audio bitrate element is not enabled in output setting",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that audio bitrate is enabled in output setting";
  }
}
