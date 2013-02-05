const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.15";
#else
    return "2.15";
#endif
}
