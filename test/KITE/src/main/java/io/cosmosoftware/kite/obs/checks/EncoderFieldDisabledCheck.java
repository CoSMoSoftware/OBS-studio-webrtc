package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.OutputSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;

public class EncoderFieldDisabledCheck extends TestCheck {

  private final MainPage mainPage;
  private final OutputSettingPage outputSettingPage;

  public EncoderFieldDisabledCheck(Runner runner) {
    super(runner);
    this.setOptional(true);
    this.mainPage = new MainPage(runner);
    this.outputSettingPage = new OutputSettingPage(runner);
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.outputSettingPage.openSettingOption();
    boolean encoderAvailability = this.outputSettingPage.getEndcoderFieldAvailibility();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.outputSettingPage.discardChange();
    if (encoderAvailability) {
      throw new KiteTestException("Encoder field in Output setting is still enabled",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that the encoder drop menu is disabled in Output Setting UI";
  }
}
