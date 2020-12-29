package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.StreamSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class StreamTypeMillicastCheck extends TestCheck {

  private final String EXPECTED_VALUE = "Millicast";
  private final MainPage mainPage;
  private final StreamSettingPage streamSettingPage;

  public StreamTypeMillicastCheck(Runner runner) {
    super(runner);
    this.setOptional(true);
    this.mainPage = new MainPage(runner);
    this.streamSettingPage = new StreamSettingPage(runner);
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.streamSettingPage.openSettingOption();
    List<String> streamTypes = this.streamSettingPage.getSteamType();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.streamSettingPage.discardChange();
    if (streamTypes.size() > 1) {
      throw new KiteTestException("There are more than 1 stream type options", Status.FAILED);
    }

    if (streamTypes.isEmpty()) {
      throw new KiteTestException("There is no stream type options", Status.FAILED);
    }

    if (!streamTypes.get(0).equals(EXPECTED_VALUE)) {
      throw new KiteTestException(
          "The stream type option was not " + EXPECTED_VALUE + " but :" + streamTypes.get(0),
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that there's only Millicast stream option";
  }
}
