package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;
import java.util.List;

public class RecordingButtonCheck extends TestCheck {

  private final MainPage mainPage;

  public RecordingButtonCheck(Runner runner, MainPage mainPage) {
    super(runner);
    this.setOptional(true);
    this.mainPage = mainPage;
  }

  @Override
  protected void step() throws KiteTestException {
    List<String> buttonLabels = this.mainPage.getButtonLabels();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    if (buttonLabels != null && buttonLabels.contains("Recording")) {
      throw new KiteTestException("The button recording still exists in main menu", Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "-> Recording button in main menu has been removed";
  }
}
