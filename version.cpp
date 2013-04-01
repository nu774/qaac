const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.18";
#else
    return "2.18";
#endif
}
