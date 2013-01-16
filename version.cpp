const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.12";
#else
    return "2.12";
#endif
}
