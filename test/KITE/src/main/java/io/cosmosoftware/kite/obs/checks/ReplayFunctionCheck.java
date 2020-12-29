package io.cosmosoftware.kite.obs.checks;

import io.cosmosoftware.kite.interfaces.Runner;
import io.cosmosoftware.kite.obs.pages.MainPage;
import io.cosmosoftware.kite.obs.pages.settings.GeneralSettingPage;
import io.cosmosoftware.kite.obs.pages.settings.HotkeysSettingPage;
import io.cosmosoftware.kite.obs.pages.settings.OutputSettingPage;
import io.cosmosoftware.kite.steps.StepPhase;
import io.cosmosoftware.kite.steps.TestCheck;
import io.cosmosoftware.kite.steps.TestStep;
import java.util.ArrayList;
import java.util.List;

public class ReplayFunctionCheck extends TestCheck {

  private final Runner runner;
  private final MainPage mainPage;
  private final GeneralSettingPage generalSettingPage;
  private final OutputSettingPage outputSettingPage;
  private final HotkeysSettingPage hotkeysSettingPage;

  public ReplayFunctionCheck(Runner runner) {
    super(runner);
    this.setOptional(true);
    this.runner = runner;
    this.mainPage = new MainPage(runner);
    this.generalSettingPage = new GeneralSettingPage(runner);
    this.outputSettingPage = new OutputSettingPage(runner);
    this.hotkeysSettingPage = new HotkeysSettingPage(runner);
  }

  @Override
  protected void step() {
    List<TestStep> subSteps = this.prepareSubSteps();
    for (TestStep step : subSteps) {
      step.processTestStep(StepPhase.DEFAULT, this.report, false);
    }
  }

  @Override
  public String stepDescription() {
    return "Verify that replaying related functions have been removed";
  }

  private List<TestStep> prepareSubSteps() {
    List<TestStep> subSteps = new ArrayList<>();
    subSteps.add(new ReplayButtonCheck(runner, this.mainPage));
    subSteps
        .add(new ReplayGeneralOutputOptionCheck(runner, this.mainPage, this.generalSettingPage));
    subSteps.add(new ReplayOutputSectionCheck(runner, this.mainPage, this.outputSettingPage));
    subSteps.add(new ReplayHotkeyOptionCheck(runner, this.mainPage, this.hotkeysSettingPage));
    return subSteps;
  }
}
