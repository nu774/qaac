const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.44";
#else
    return "2.44";
#endif
}
