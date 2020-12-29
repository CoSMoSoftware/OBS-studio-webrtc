package io.cosmosoftware.kite.obs.pages.elements;

import java.util.HashMap;

public abstract class PageElements {

  // MAIN PAGE ITEMS
  public static final String MAIN_PAGE_BUTTON = "MAIN_PAGE_BUTTON";
  public static final String MAIN_PAGE_START_STREAMING_BTN = "MAIN_PAGE_START_STREAMING_BTN";
  public static final String MAIN_PAGE_SETTING_BTN = "MAIN_PAGE_SETTING_BTN";
  public static final String MAIN_PAGE_STATUS_TEXT = "MAIN_PAGE_STATUS_TEXT";
  public static final String MAIN_PAGE_FRAME_STATUS = "MAIN_PAGE_FRAME_STATUS";
  public static final String MAIN_PAGE_LIVE_STATUS = "MAIN_PAGE_LIVE_STATUS";
  public static final String MAIN_PAGE_REC_STATUS = "MAIN_PAGE_REC_STATUS";
  public static final String MAIN_PAGE_CPU_STATUS = "MAIN_PAGE_CPU_STATUS";
  public static final String MAIN_PAGE_STATS = "MAIN_PAGE_STATS";
  public static final String MAIN_PAGE_EXIT_BTN = "MAIN_PAGE_EXIT_BTN";
  // SETTING OPTION ITEMS
  public static final String GENERAL_OPT = "GENERAL_OPT";
  public static final String GENERAL_GROUP_ELEMENT = "GENERAL_GROUP_ELEMENT";

  // STREAM SETTINGS
  public static final String STREAM_OPT = "STREAM_OPT";
  public static final String STREAM_TYPE_DROPDOWN = "STREAM_TYPE_DROPDOWN";
  public static final String STREAM_TYPE_OPTION = "STREAM_TYPE_OPTION";

  public static final String RTMP_SERVER_NAME = "RTMP_SERVER_NAME";
  public static final String RTMP_PUBLISHING_KEY = "RTMP_PUBLISHING_KEY";

  public static final String STREAM_NAME = "STREAM_NAME";
  public static final String STREAM_PUBLISHING_TOKEN = "STREAM_PUBLISHING_TOKEN";

  public static final String STREAM_CODEC_DROPDOWN = "STREAM_CODEC_DROPDOWN";
  public static final String STREAM_CODEC = "STREAM_CODEC";
  // OUTPUT SETTINGS
  public static final String OUTPUT_OPT = "OUTPUT_OPT";
  public static final String OUTPUT_MODES = "OUTPUT_MODES";
  public static final String OUTPUT_MODES_OPTIONS = "OUTPUT_MODES_OPTIONS";
  public static final String OUTPUT_GROUP_ELEMENT = "OUTPUT_GROUP_ELEMENT";
  public static final String OUTPUT_VIDEO_BITRATE = "OUTPUT_VIDEO_BITRATE";
  public static final String OUTPUT_AUDIO_BITRATE = "OUTPUT_AUDIO_BITRATE";
  public static final String ENDCODER = "ENDCODER";
  // AUDIO SETTINGS
  public static final String AUDIO_OPT = "AUDIO_OPT";
  // VIDEO SETTINGS
  public static final String VIDEO_OPT = "VIDEO_OPT";
  public static final String BASE_RESOLUTION = "BASE_RESOLUTION";
  public static final String OUTPUT_RESOLUTION = "OUTPUT_RESOLUTION";
  public static final String DOWNSCALED_FILTER = "DOWNSCALED_FILTER";
  public static final String FPS = "FPS";
  // HOTKEY SETTINGS
  public static final String HOTKEY_OPT = "HOTKEY_OPT";
  public static final String HOTKEY_OPTION = "HOTKEY_OPTION";
  // ADVANCED SETTINGS
  public static final String ADVANCED_OPT = "ADVANCED_OPT";
  public static final String ADVANCED_GROUP_ELEMENT = "ADVANCED_GROUP_ELEMENT";

  public static final String OK_BTN = "OK_BTN";
  public static final String CANCEL_BTN = "CANCEL_BTN";
  public static final String APPLY_BTN = "APPLY_BTN";


  public HashMap<String, String> populateElement(String os) {
    HashMap<String, String> elements = new HashMap<>();
    if (os.equals("WINDOWS")) {
      getWindowsElements(elements);
    } else if (os.equals("MAC")) {
      getMacElements(elements);
    } else {
      getLinuxElements(elements);
    }
    return elements;
  }

  abstract protected void getWindowsElements(HashMap<String, String> elements);

  abstract protected void getMacElements(HashMap<String, String> elements);

  abstract protected void getLinuxElements(HashMap<String, String> elements);
}
