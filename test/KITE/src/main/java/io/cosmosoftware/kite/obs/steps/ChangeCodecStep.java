package io.cosmosoftware.kite.obs.steps;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.StreamSettingPage;
import io.cosmosoftware.kite.steps.TestStep;
import io.cosmosoftware.kite.util.ReportUtils;

public class ChangeCodecStep extends TestStep {

  private final MainPage mainPage;
  private final StreamSettingPage streamSettingPage;
  private final String codec;


  public ChangeCodecStep(Runner runner, String codec) {
    super(runner);
    this.mainPage = new MainPage(runner);
    this.streamSettingPage = new StreamSettingPage(runner);
    this.codec = codec;
  }

  @Override
  protected void step() throws KiteTestException {
    mainPage.openSettings();
    try {
      streamSettingPage.openSettingOption();
      streamSettingPage.pickCodec(this.codec);
      this.reporter.screenshotAttachment(this.report, this.getClassName(), ReportUtils
          .saveScreenshotPNG(this.webDriver));
    } catch (Exception e) {
      throw e;
    } finally {
      streamSettingPage.applyChange();
    }
  }

  @Override
  public String stepDescription() {
    return "Change codec to : " + this.codec;
  }

}
