const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.23";
#else
    return "1.12";
#endif
}
