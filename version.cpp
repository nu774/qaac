const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.40";
#else
    return "2.40";
#endif
}
