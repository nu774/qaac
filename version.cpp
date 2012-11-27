const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.05";
#else
    return "2.05";
#endif
}
