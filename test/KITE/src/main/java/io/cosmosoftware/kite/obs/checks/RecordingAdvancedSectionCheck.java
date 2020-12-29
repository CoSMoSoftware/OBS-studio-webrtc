package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.AdvancedSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class RecordingAdvancedSectionCheck extends TestCheck {

  private final MainPage mainPage;
  private final AdvancedSettingPage advancedSettingPage;

  public RecordingAdvancedSectionCheck(Runner runner, MainPage mainPage,
      AdvancedSettingPage advancedSettingPage) {
    super(runner);
    this.setOptional(true);
    this.mainPage = mainPage;
    this.advancedSettingPage = advancedSettingPage;
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.advancedSettingPage.openSettingOption();
    List<String> groupLabels = this.advancedSettingPage.getSettingGroupLabels();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.advancedSettingPage.discardChange();
    if (groupLabels.isEmpty()) {
      throw new KiteTestException("Could not get the group label in advanced setting",
          Status.FAILED);
    }
    if (groupLabels.contains("Recording")) {
      throw new KiteTestException("The recording section still exists in advanced setting",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "-> Recording section in advanced setting has been removed";
  }
}
