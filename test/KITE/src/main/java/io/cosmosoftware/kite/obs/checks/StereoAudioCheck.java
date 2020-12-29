package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.exception.KiteTestException;
import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.OBSViewerPage;
import io.cosmosoftware.kite.report.Status;
import io.cosmosoftware.kite.steps.TestStep;

public class StereoAudioCheck extends TestStep {

  private final OBSViewerPage obsViewerPage;

  public StereoAudioCheck(Runner runner, OBSViewerPage obsViewerPage) {
    super(runner);
    this.obsViewerPage = obsViewerPage;
    this.setOptional(true);
  }

  @Override
  protected void step() throws KiteTestException {
    String offer = obsViewerPage.getSdpMessage("Local");
    String answer = obsViewerPage.getSdpMessage("Remote");
    this.reporter.textAttachment(this.report, "OBS SDP offer: ", offer, "plain");
    this.reporter.textAttachment(this.report, "OBS SDP answer: ", answer, "plain");
    checkContainLine("offer", offer, "opus/48000/2");
    checkContainLine("offer", offer, "stereo=1");
    checkContainLine("offer", offer, "sprop-stereo=1");
    checkContainLine("answer", answer, "opus/48000/2");
    checkContainLine("answer", answer, "stereo=1");
    checkContainLine("answer", answer, "sprop-stereo=1");
  }

  private void checkContainLine(String type, String sdpMessage, String line)
      throws KiteTestException {
    if (!sdpMessage.contains(line)) {
      throw new KiteTestException("Could not find '" + line + "' in the sdp message (" + type + ")",
          Status.FAILED);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that audio is stereo (by sdp)";
  }
}
