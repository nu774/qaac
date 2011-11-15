const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.12";
#else
    return "1.04";
#endif
}
