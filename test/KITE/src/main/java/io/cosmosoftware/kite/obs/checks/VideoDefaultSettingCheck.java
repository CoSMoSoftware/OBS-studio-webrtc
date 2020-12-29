package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.VideoSettingPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.util.ReportUtils;

public class VideoDefaultSettingCheck extends TestCheck {

  private final MainPage mainPage;
  private final VideoSettingPage videoSettingPage;
  private final String EXPECTED_RESOLUTION = "1920x1080";
  private final String EXPECTED_FILTER = "Bilinear";
  private final String EXPECTED_PFS = "30";

  public VideoDefaultSettingCheck(Runner runner) {
    super(runner);
    this.setOptional(true);
    this.mainPage = new MainPage(runner);
    this.videoSettingPage = new VideoSettingPage(runner);
  }

  @Override
  protected void step() throws KiteTestException {
    this.mainPage.openSettings();
    this.videoSettingPage.openSettingOption();
    String baseRes = videoSettingPage.getBaseResolution();
    String outputRes = videoSettingPage.getOutputResolution();
    String downscaledFilter = videoSettingPage.getDownscaledFilter();
    String fps = videoSettingPage.getFps();
    this.reporter.screenshotAttachment(this.report, this.getClassName(),
        ReportUtils.saveScreenshotPNG(this.webDriver));
    this.reporter.textAttachment(this.report, "Obtained values",
        "baseRes: " + baseRes
            + ", outputRes:" + outputRes
            + ", downscaled filter: " + downscaledFilter
            + ", fps: " + fps,
        "plain");
    this.videoSettingPage.discardChange();

    // todo : verify with default value:
    // base res: 1920x1080
    // output res : 1920x1080
    // downscale : Bilinear
    // fps : 30
    if (!baseRes.equals(EXPECTED_RESOLUTION)) {
      throw new KiteTestException(
          "The base resolution default value was not correct (" + baseRes + "), expected: "
              + EXPECTED_RESOLUTION, Status.FAILED);
    }
    if (!outputRes.equals(EXPECTED_RESOLUTION)) {
      throw new KiteTestException(
          "The output resolution default value was not correct (" + outputRes + "), expected: "
              + EXPECTED_RESOLUTION, Status.FAILED);
    }
    if (!downscaledFilter.startsWith(EXPECTED_FILTER)) {
      throw new KiteTestException(
          "The downscaled filter default value was not correct (" + downscaledFilter
              + "), expected: " + EXPECTED_FILTER + "..", Status.FAILED);
    }
    if (!fps.equals(EXPECTED_PFS)) {
      throw new KiteTestException(
          "The fps default value was not correct (" + fps + "), expected: " + EXPECTED_PFS,
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that the default settings for video are correct";
  }
}
