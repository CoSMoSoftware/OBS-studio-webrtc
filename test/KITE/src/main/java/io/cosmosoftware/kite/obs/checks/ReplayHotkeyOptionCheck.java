package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.HotkeysSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class ReplayHotkeyOptionCheck extends TestCheck {

  private final MainPage mainPage;
  private final HotkeysSettingPage hotkeysSettingPage;

  public ReplayHotkeyOptionCheck(Runner runner, MainPage mainPage,
      HotkeysSettingPage hotkeysSettingPage) {
    super(runner);
    this.setOptional(true);
    this.mainPage = mainPage;
    this.hotkeysSettingPage = hotkeysSettingPage;
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.hotkeysSettingPage.openSettingOption();
    List<String> groupLabels = this.hotkeysSettingPage.getHotKeyOptions();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.hotkeysSettingPage.discardChange();
    if (groupLabels.isEmpty()) {
      throw new KiteTestException("Could not get the option in hot keys setting", Status.FAILED);
    }
    if (groupLabels.contains("Replay")) {
      throw new KiteTestException("The Replay option still exists in hot keys setting",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "-> Replay option in hot keys setting has been removed";
  }
}
