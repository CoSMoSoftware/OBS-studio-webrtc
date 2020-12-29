package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.OutputSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class OutputModeCheck extends TestCheck {

  private final MainPage mainPage;
  private final OutputSettingPage outputSettingPage;

  public OutputModeCheck(Runner runner) {
    super(runner);
    this.setOptional(true);
    this.mainPage = new MainPage(runner);
    this.outputSettingPage = new OutputSettingPage(runner);
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.outputSettingPage.openSettingOption();
    List<String> outputModes = this.outputSettingPage.getOutputModes();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.outputSettingPage.discardChange();
    if (outputModes.size() != 1) {
      throw new KiteTestException(
          "The number of output mode options on the output setting UI is not correct",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that only simple output mode is left on the UI";
  }
}
