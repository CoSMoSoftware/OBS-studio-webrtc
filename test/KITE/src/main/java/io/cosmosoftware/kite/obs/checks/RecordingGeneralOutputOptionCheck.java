package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.GeneralSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class RecordingGeneralOutputOptionCheck extends TestCheck {

  private final MainPage mainPage;
  private final GeneralSettingPage generalSettingPage;

  public RecordingGeneralOutputOptionCheck(Runner runner, MainPage mainPage,
      GeneralSettingPage generalSettingPage) {
    super(runner);
    this.setOptional(true);
    this.mainPage = mainPage;
    this.generalSettingPage = generalSettingPage;
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.generalSettingPage.openSettingOption();
    List<String> options = this.generalSettingPage.getOutputOptions();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.generalSettingPage.discardChange();
    if (options.isEmpty()) {
      throw new KiteTestException("Could not get the output option in general setting",
          Status.FAILED);
    }
    for (String option : options) {
      if (option.toLowerCase().contains("recording")) {
        throw new KiteTestException("The Recording option still exists in general setting",
            Status.FAILED);
      }
    }
  }

  @Override
  public String stepDescription() {
    return "-> Recording option in general setting has been removed";
  }
}
