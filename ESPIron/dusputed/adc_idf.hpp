/**
 * @brief Class provides an instance of ESP32 oneshot ADC sampler with calibration
 * 
 */
class ADCSensor_OneShot {
  // do calibration
  bool _cal_enabled;

  //static adc_unit_t _adc_units[SOC_ADC_PERIPH_NUM];
  // ADC Unit
  adc_unit_t _unit;
  // ADC channel to sample
  adc_channel_t _chan;
  static adc_oneshot_unit_handle_t _adc_handle[SOC_ADC_PERIPH_NUM];
  adc_cali_handle_t _cal_handle{nullptr};

  esp_err_t _adc_calibration_init();
  void _adc_calibration_deinit();

  int32_t _convertmv();

protected:
  int adc_raw, adc_voltage;

public:
  ~ADCSensor_OneShot();

  // initialize ADC
  esp_err_t init(int gpio, bool cal = true);

  bool calibarationEnabled() const { return _cal_enabled; }

  /**
   * @brief returns calibrated mV reading
   * on read error returns -1
   * on calibration error returns readmv()
   * 
   * @return int32_t 
   */
  int32_t read();

  /**
   * @brief returns raw adc reading
   * on error returns -1
   * 
   * @return int32_t 
   */
  int32_t readraw();

  /**
   * @brief returns uncalibrated mV
   * on error returns -1
   * 
   * @return int32_t 
   */
  int32_t readmv();

};