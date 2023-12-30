#include <exortix-project-1_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include <Arduino.h>

String bb_structToJSON(ei_impulse_result_bounding_box_t *bb)
{
  if (bb->label == NULL)
  {
    return "{}";
  }
  String json = "{";
  json += "\"label\": \"" + String(bb->label) + "\",";
  json += "\"x\": " + String(bb->x) + ",";
  json += "\"y\": " + String(bb->y) + ",";
  json += "\"width\": " + String(bb->width) + ",";
  json += "\"height\": " + String(bb->height) + ",";
  json += "\"value\": " + String(bb->value);
  json += "}";
  return json;
}

String bb_arrayToJSON(ei_impulse_result_bounding_box_t *bb, size_t bb_count)
{
  String json = "[";
  for (size_t ix = 0; ix < bb_count; ix++)
  {
    json += bb_structToJSON(&bb[ix]);
    if (ix < bb_count - 1)
    {
      json += ",";
    }
  }
  json += "]";
  return json;
}