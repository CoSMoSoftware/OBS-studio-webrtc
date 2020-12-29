package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class RecordingStatusCheck extends TestCheck {

  private final MainPage mainPage;

  public RecordingStatusCheck(Runner runner, MainPage mainPage) {
    super(runner);
    this.setOptional(true);
    this.mainPage = mainPage;
  }

  @Override
  protected void step() throws KiteTestException {
    List<String> statusTexts = this.mainPage.getStatusTexts();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    for (String text : statusTexts) {
      if (text != null && text.contains("REC")) {
        throw new KiteTestException("The status recording still exists in main menu",
            Status.FAILED);
      }
    }
  }

  @Override
  public String stepDescription() {
    return "-> Recording status in main menu has been removed";
  }
}
