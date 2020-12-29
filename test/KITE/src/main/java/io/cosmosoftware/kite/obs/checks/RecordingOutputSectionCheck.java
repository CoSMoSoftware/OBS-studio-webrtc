package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.OutputSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class RecordingOutputSectionCheck extends TestCheck {

  private final MainPage mainPage;
  private final OutputSettingPage outputSettingPage;

  public RecordingOutputSectionCheck(Runner runner, MainPage mainPage,
      OutputSettingPage outputSettingPage) {
    super(runner);
    this.setOptional(true);
    this.mainPage = mainPage;
    this.outputSettingPage = outputSettingPage;
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.outputSettingPage.openSettingOption();
    List<String> groupLabels = this.outputSettingPage.getSettingGroupLabels();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.outputSettingPage.discardChange();
    if (groupLabels.isEmpty()) {
      throw new KiteTestException("Could not get the group label in output setting", Status.FAILED);
    }
    if (groupLabels.contains("Recording")) {
      throw new KiteTestException("The recording section still exists in output setting",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "-> Recording section in output setting has been removed";
  }
}
