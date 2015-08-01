const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.52";
#else
    return "2.52";
#endif
}
