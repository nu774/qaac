const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.25";
#else
    return "2.25";
#endif
}
